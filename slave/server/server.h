#pragma once
#include <kea/dhcp++/dhcp4.h>
#include <kea/dhcp++/pkt.h>
#include <kea/dhcp++/option.h>
#include <kea/dhcp++/option_string.h>
#include <kea/dhcp++/option4_client_fqdn.h>
#include <kea/dhcp++/option_custom.h>
#include <kea/dhcp++/subnet.h>
#include <kea/server/subnet_mgr.h>
#include <kea/server/base_host_data_source.h>
#include <folly/MPMCQueue.h>
#include <kea/util/io_address.h>
#include <kea/client/client_context.h>

using namespace kea::dhcp;

namespace kea {
namespace server {

typedef folly::MPMCQueue<PktPtr> PktQueue;
using kea::client::ClientContextPtr;
using kea::client::ClientContext;

class Dhcpv4Srv {
public:
    typedef enum {
        FORBIDDEN,
        MANDATORY,
        OPTIONAL
    } RequirementLevel;

    Dhcpv4Srv(SubnetMgr* subnet_mgr, BaseHostDataSource* host_mgr, PktQueue& out_queue);

    void stop();
    void processPacket(PktPtr query);
    uint16_t getPort() const { return (port_); }
    bool useBroadcast() const { return (false); }

private:
    bool accept(const Pkt&) const;
    bool acceptDirectRequest(const Pkt& ) const;
    bool acceptMessageType(const Pkt& ) const;
    bool acceptServerId(const Pkt& ) const;
    static void sanityCheck(const Pkt& , RequirementLevel);

    void processRequest(PktPtr);
    void processRelease(PktPtr);
    void processDecline(PktPtr);
    void processInform(PktPtr);

    Subnet* selectSubnet(const Pkt& query) const;
    void classifyPacket(Pkt& pkt);

    void denyRequest(Pkt& query);
    void declineLease(Pkt& decline);
    void classifyByVendor(Pkt& pkt, std::string& classes);
    void beforePktSent(Pkt* query, Pkt* rsp);  
    void onRPCFinish(ClientContextPtr client_ctx);
    void onPingFinish(ClientContextPtr client_ctx);
    void assignLease(ClientContextPtr client_ctx, const Subnet& subnet);
    void allocateLease(ClientContextPtr client_ctx);
    void allocateSubnet(ClientContextPtr client_ctx);

    uint16_t port_;  
    bool use_bcast_;
    SubnetMgr* subnet_mgr_;
    BaseHostDataSource* host_mgr_;
    PktQueue& out_queue_;
};
}; 
};
