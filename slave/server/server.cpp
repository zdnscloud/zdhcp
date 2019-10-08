#include <kea/server/server.h>
#include <kea/dhcp++/dhcp4.h>
#include <kea/dhcp++/duid.h>
#include <kea/dhcp++/hwaddr.h>
#include <kea/nic/iface_mgr.h>
#include <kea/dhcp++/option4_addrlst.h>
#include <kea/dhcp++/option_int.h>
#include <kea/dhcp++/option_int_array.h>
#include <kea/dhcp++/option_vendor.h>
#include <kea/dhcp++/option_string.h>
#include <kea/dhcp++/pkt.h>
#include <kea/dhcp++/vendor_option_defs.h>
#include <kea/server/subnet_selector.h>
#include <kea/server/hosts_in_mem.h>
#include <kea/server/client_class_manager.h>
#include <kea/hooks/hooks.h>
#include <kea/hooks/hooks_manager.h>
#include <kea/hooks/callout_handle.h>
#include <kea/ping/ping.h>
#include <kea/rpc/rpc_allocate_engine.h>
#include <kea/server/response_gen.h>
#include <kea/logging/logging.h>
#include <kea/statistics/pkt_statistic.h>
#include <functional>   


using namespace kea;
using namespace kea::dhcp;
using namespace kea::nic;
using namespace kea::hooks;
using namespace kea::pinger;
using namespace kea::logging;
using namespace kea::statis;
using namespace std;

namespace kea {
namespace server {

const uint32_t MAX_RETRY_COUNT = 5;
const uint32_t DECLINE_CONFLICT_TRANS_ID = 1234;
const std::string VENDOR_CLASS_PREFIX("VENDOR_CLASS_");

struct Dhcp4Hooks {
    int hook_index_pkt4_receive_;   
    int hook_index_pkt4_send_;     

    Dhcp4Hooks() {
        hook_index_pkt4_receive_   = HooksManager::instance().registerHook("pkt4_receive");
        hook_index_pkt4_send_      = HooksManager::instance().registerHook("pkt4_send");
    }    
};

Dhcp4Hooks Hooks;

Dhcpv4Srv::Dhcpv4Srv(SubnetMgr* subnet_mgr,
                     BaseHostDataSource* host_mgr,
                     PktQueue& out_queue)
    : subnet_mgr_(subnet_mgr), 
      host_mgr_(host_mgr),
      out_queue_(out_queue){
}

Subnet*
Dhcpv4Srv::selectSubnet(const Pkt& query) const {
    SubnetSelector selector;
    selector.ciaddr_ = query.getCiaddr();
    selector.giaddr_ = query.getGiaddr();
    selector.local_address_ = query.getLocalAddr();
    selector.remote_address_ = query.getRemoteAddr();
    selector.client_classes_ = query.getClasses();
    selector.iface_name_ = query.getIface();

    const Option* rai = query.getOption(DHO_DHCP_AGENT_OPTIONS);
    if (rai) {
        const OptionCustom* rai_custom = dynamic_cast<const OptionCustom *>(rai);
        if (rai_custom) {
            const Option* link_select = rai_custom->getOption(RAI_OPTION_LINK_SELECTION);
            if (link_select) {
                OptionBuffer link_select_buf = link_select->getData();
                if (link_select_buf.size() == sizeof(uint32_t)) {
                    selector.option_select_ = IOAddress::fromBytes(AF_INET, link_select_buf.data());
                }
            }
        }
    } else {
        const Option* sbnsel = query.getOption(DHO_SUBNET_SELECTION);
        if (sbnsel) {
            const OptionCustom* oc = dynamic_cast<const OptionCustom*>(sbnsel);
            if (oc) {
                selector.option_select_ = oc->readAddress();
            }
        }
    }

    return const_cast<Subnet*>(subnet_mgr_->selectSubnet(selector));
}

void 
Dhcpv4Srv::beforePktSent(Pkt* query, Pkt* rsp) {
    if (HooksManager::instance().calloutsPresent(Hooks.hook_index_pkt4_send_)) {
        std::unique_ptr<CalloutHandle> callout_handle = HooksManager::instance().createCalloutHandle();
        callout_handle->setArgument("query4", query);
        callout_handle->setArgument("response4", rsp);
        HooksManager::instance().callCallouts(Hooks.hook_index_pkt4_send_, *callout_handle);
        if (callout_handle->getStatus() == CalloutHandle::NEXT_STEP_SKIP) {
            return;
        }
    }
    Statistics::instance().count_send(query, rsp);
}

void
Dhcpv4Srv::stop() {
}

void
Dhcpv4Srv::processPacket(PktPtr query) {
    try{
        query->unpack();
    } catch (const std::exception& e) {
        return;
    }

    classifyPacket(*query);
    if (!accept(*query)) {
        logError("Dhcpv4Srv ", "Query cann't be accepted");
        return; 
    }

    if (HooksManager::instance().calloutsPresent(Hooks.hook_index_pkt4_receive_)) {
        std::unique_ptr<CalloutHandle> callout_handle = HooksManager::instance().createCalloutHandle();
        callout_handle->setArgument("query4", query.get());
        HooksManager::instance().callCallouts(Hooks.hook_index_pkt4_receive_, *callout_handle);
        if (callout_handle->getStatus() == CalloutHandle::NEXT_STEP_SKIP) {
            return;
        }
    }
    Statistics::instance().count_recv(query.get());

    try {
        logInfo("Dhcpv4Srv ", query->toText().c_str());
        switch (query->getType()) {
            case DHCPDISCOVER:
            case DHCPREQUEST:
                processRequest(std::move(query));
                break;

            case DHCPRELEASE:
                processRelease(std::move(query));
                break;

            case DHCPDECLINE:
                processDecline(std::move(query));
                break;

            case DHCPINFORM:
                processInform(std::move(query));
                break;

            default:
                ;
        }
    } catch (const std::exception& e) {
        logError("Dhcpv4Srv ", "Handle query get exception: $0", e.what());
    }
}

void Dhcpv4Srv::allocateLease(ClientContextPtr client_ctx) {
    using namespace std::placeholders;
    if (client_ctx->getRetryCount() > MAX_RETRY_COUNT) {
        return;
    }


    kea::rpc::RpcAllocateEngine::instance().allocateAddr(std::move(client_ctx),
                                   std::bind(&Dhcpv4Srv::onRPCFinish, this, _1));
}

void 
Dhcpv4Srv::onRPCFinish(ClientContextPtr client_ctx) {
    using namespace std::placeholders;

    IOAddress allocated_addr = client_ctx->getYourAddr();
    if (allocated_addr.isV4Bcast() || allocated_addr.isV4Zero()) {
        logDebug("Dhcpv4Srv ", "Send NAK when onRPCFinish got ip: $0", allocated_addr.toText());
        denyRequest(client_ctx->getQuery());
        return;
    }

    if (client_ctx->getQueryType() == DHCPDISCOVER && client_ctx->getQuery().getCiaddr() != allocated_addr) {
        Pinger::instance().ping(std::move(client_ctx),
                                std::bind(&Dhcpv4Srv::onPingFinish, this, _1));
    } else {
        allocateSubnet(std::move(client_ctx));
    }
}

void 
Dhcpv4Srv::onPingFinish(ClientContextPtr client_ctx) {
    if (client_ctx->isRequestAddrConflict()) {
        logWarning("Dhcpv4Srv ", "IP: $0 which discover allocated has being used", client_ctx->getYourAddr().toText().c_str());
        auto decline_ip = client_ctx->getYourAddr();
        auto subnet = subnet_mgr_->selectSubnet(decline_ip, client_ctx->getQuery().getClasses());
        if (subnet != nullptr) {
            PktPtr decline(new Pkt(DHCPCONFLICTIP, DECLINE_CONFLICT_TRANS_ID));
            decline->setCiaddr(decline_ip);
            kea::rpc::RpcAllocateEngine::instance().allocateAddr(ClientContextPtr(new ClientContext(std::move(decline), *subnet)), nullptr);
        } else {
            logWarning("Dhcpv4Srv ", "Not found subnet when process decline with IP $0", decline_ip.toText()); 
        }
        client_ctx->addRetryCount();
        allocateLease(std::move(client_ctx));
    } else {
        allocateSubnet(std::move(client_ctx));
    }
}

void 
Dhcpv4Srv::allocateSubnet(ClientContextPtr client_ctx) {
    auto shared_subnet_id = client_ctx->getSharedSubnetID();
    if ( shared_subnet_id  && shared_subnet_id != client_ctx->getSubnetID()) {
        auto subnet = subnet_mgr_->getSubnet(shared_subnet_id);
        if (subnet == nullptr) {
            logWarning("Dhcpv4Srv ", "Not found shared subnet by subnet_id $0", shared_subnet_id);
            denyRequest(client_ctx->getQuery());
        } else {
            assignLease(std::move(client_ctx), *subnet);
        }
    } else {
        assignLease(std::move(client_ctx), client_ctx->getSubnet());
    }
}

void
Dhcpv4Srv::assignLease(ClientContextPtr client_ctx, const Subnet& subnet){
    PktPtr resp = genAckResponse(client_ctx->getQuery(), client_ctx->getYourAddr(), subnet);
    resp->pack();
    beforePktSent(&client_ctx->getQuery(), resp.get());
    out_queue_.blockingWrite(std::move(resp));
}

void 
Dhcpv4Srv::processRequest(PktPtr query) {
    if (query->getType() == DHCPDISCOVER) {
        sanityCheck(*query, FORBIDDEN);
    }

    auto subnet = selectSubnet(*query);
    if (subnet == nullptr) {
        logWarning("Dhcpv4Srv ", "Not found subnet when process discover or request by query $0", query->toText().c_str());
        denyRequest(*query);
    } else {
        allocateLease(ClientContextPtr(new ClientContext(std::move(query), *subnet)));
    }
}

void 
Dhcpv4Srv::denyRequest(Pkt& req) {
    PktPtr resp = genNakResponse(req);
    resp->pack();
    beforePktSent(&req, resp.get());
    out_queue_.blockingWrite(std::move(resp));
}

void Dhcpv4Srv::processRelease(PktPtr release) {
    auto subnet = subnet_mgr_->selectSubnet(release->getCiaddr(), release->getClasses());
    if (subnet != nullptr) {
        kea::rpc::RpcAllocateEngine::instance().allocateAddr(ClientContextPtr(new ClientContext(std::move(release), *subnet)), nullptr);
    } else {
        logWarning("Dhcpv4Srv ", "Not found subnet when process release with Ciaddr $0", release->getCiaddr().toText());
    }
}

void 
Dhcpv4Srv::processDecline(PktPtr decline) {
    const OptionCustom* opt_requested_address = dynamic_cast<const OptionCustom*> (decline->getOption(DHO_DHCP_REQUESTED_ADDRESS));
    if (opt_requested_address) {
        IOAddress request_ip(opt_requested_address->readAddress());
        auto subnet = subnet_mgr_->selectSubnet(request_ip, decline->getClasses());
        if (subnet != nullptr) {
            kea::rpc::RpcAllocateEngine::instance().allocateAddr(ClientContextPtr(new ClientContext(std::move(decline), *subnet)), nullptr);
        } else {
            logWarning("Dhcpv4Srv ", "Not found subnet when process decline with IP $0", request_ip.toText()); 
        }
    } else {
        logError("Dhcpv4Srv ", "Decline failed with error: Not found request ip");
    }
}

void
Dhcpv4Srv::processInform(PktPtr inform) {
    sanityCheck(*inform, FORBIDDEN);
    auto subnet = subnet_mgr_->selectSubnet(inform->getCiaddr(), inform->getClasses());
    if (subnet == nullptr) {
        logWarning("Dhcpv4Srv ", "Not found subnet when process inform with Ciaddr $0", inform->getCiaddr().toText());
        denyRequest(*inform);
    } else {
        PktPtr resp = genAckResponse(*inform, IOAddress(0), *subnet); 
        resp->pack();
        beforePktSent(inform.get(), resp.get());
        out_queue_.blockingWrite(std::move(resp));
    }
}

bool 
Dhcpv4Srv::accept(const Pkt& query) const {
    if (!acceptMessageType(query)) {
        logError("Dhcpv4Srv ", "Accept failed, query type $0 is invalid", query.getType());
        return (false);
    }
    if (!acceptDirectRequest(query)) {
        logError("Dhcpv4Srv ", "Accept failed, remote_addr and ciaddr is zero for inform query");
        return (false);
    }
    if (!acceptServerId(query)) {
        logError("Dhcpv4Srv ", "Accept failed, server identifier is invalid");
        return (false);
    }
    return (true);
}

bool 
Dhcpv4Srv::acceptDirectRequest(const Pkt& pkt) const {
    if (pkt.isRelayed()) {
        return (true);
    }

    if (pkt.getType() == DHCPINFORM && 
        pkt.getRemoteAddr().isV4Zero() &&
        pkt.getCiaddr().isV4Zero()) {
        return (false);
    }
    return (true);
}

bool 
Dhcpv4Srv::acceptMessageType(const Pkt& query) const {
    int type = query.getType();
    return (type == DHCPDISCOVER || type == DHCPREQUEST ||
            type == DHCPRELEASE || type == DHCPDECLINE || type == DHCPINFORM);
}

bool 
Dhcpv4Srv::acceptServerId(const Pkt& query) const {
    const Option* option = query.getOption(DHO_DHCP_SERVER_IDENTIFIER);
    if (!option) {
        return (true);
    }

    // Server identifier is present. Let's convert it to 4-byte address
    // and try to match with server identifiers used by the server.
    const OptionCustom* option_custom = dynamic_cast<const OptionCustom *>(option);
    if (!option_custom) {
        return (false);
    }
    if (option_custom->getDataFieldsNum() != 1) {
        return (false);
    }

    IOAddress server_id = option_custom->readAddress();
    if (!server_id.isV4()) {
        return (false);
    }
    return (IfaceMgr::instance().hasOpenSocket(server_id));
}

void 
Dhcpv4Srv::sanityCheck(const Pkt& query, RequirementLevel serverid) {
    const Option* server_id = query.getOption(DHO_DHCP_SERVER_IDENTIFIER);
    switch (serverid) {
        case FORBIDDEN:
            if (server_id) {
                kea_throw(RFCViolation, "Server-id option was not expected, but "
                        << "received in "
                        << query.getName());
            }
            break;

        case MANDATORY:
            if (!server_id) {
                kea_throw(RFCViolation, "Server-id option was expected, but not "
                        " received in message "
                        << query.getName());
            }
            break;

        case OPTIONAL:
            // do nothing here
            ;
    }

    if (query.getHWAddr().hwaddr_.empty() == false) {
        return;
    }

    const Option* client_id = query.getOption(DHO_DHCP_CLIENT_IDENTIFIER);
    if (!client_id || client_id->len() == client_id->getHeaderLen()) {
        kea_throw(RFCViolation, "Missing or useless client-id and no HW address "
                " provided in message "
                << query.getName());
    }
}

void 
Dhcpv4Srv::classifyByVendor(Pkt& pkt, std::string& classes) {
    const OptionString* vendor_class = dynamic_cast<const OptionString*>
        (pkt.getOption(DHO_VENDOR_CLASS_IDENTIFIER));
    if (!vendor_class) {
        return;
    }
    if (vendor_class->getValue().find(DOCSIS3_CLASS_MODEM) != std::string::npos) {
        pkt.addClass(VENDOR_CLASS_PREFIX + DOCSIS3_CLASS_MODEM);
        classes += string(VENDOR_CLASS_PREFIX + DOCSIS3_CLASS_MODEM) + " ";
    } else if (vendor_class->getValue().find(DOCSIS3_CLASS_EROUTER) != std::string::npos) {
        pkt.addClass(VENDOR_CLASS_PREFIX + DOCSIS3_CLASS_EROUTER);
        classes += string(VENDOR_CLASS_PREFIX + DOCSIS3_CLASS_EROUTER) + " ";
    } else {
        pkt.addClass(VENDOR_CLASS_PREFIX + vendor_class->getValue());
        classes += VENDOR_CLASS_PREFIX + vendor_class->getValue();
    }
}

void 
Dhcpv4Srv::classifyPacket(Pkt& pkt) {
    std::vector<std::string> pkt_meet_classes = ClientClassManager::instance().getMatchedClass(pkt);
    for (auto& client_class : pkt_meet_classes)  {
        pkt.addClass(client_class);
    }
}

};
};
