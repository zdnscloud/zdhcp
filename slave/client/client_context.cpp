#include <kea/client/client_context.h>
#include <kea/dhcp++/option_custom.h>

namespace kea {
namespace client {

using namespace kea::dhcp;

ClientContext::ClientContext(PktPtr query, const Subnet& subnet)
    : query_(std::move(query)), 
    subnet_(subnet), 
    is_request_addr_conflict_(false),
    your_addr_(IOAddress(0)),
    retry_count_(0) {
}

std::vector<uint8_t> 
ClientContext::getClientID() const {
    const Option* opt_clientid = query_->getOption(DHO_DHCP_CLIENT_IDENTIFIER);
    if (opt_clientid) {
        return opt_clientid->getData();
    } else {
        return vector<uint8_t>();
    }
}

std::vector<uint8_t> 
ClientContext::getHWAddr() const {
    return query_->getHWAddr().hwaddr_;
}

IOAddress
ClientContext::getRequestAddr() const {
    const OptionCustom* opt_requested_address = dynamic_cast<const OptionCustom*>
        (query_->getOption(DHO_DHCP_REQUESTED_ADDRESS));
    if (opt_requested_address) {
        return opt_requested_address->readAddress();
    } else if (!query_->getCiaddr().isV4Zero()) {
        return query_->getCiaddr();
    } else {
        return IOAddress(0);
    }
}

uint8_t
ClientContext::getQueryType() const {
    return query_->getType();    
}

void
ClientContext::setRequestAddrConflict(bool conflict) {
    is_request_addr_conflict_ = conflict;
}

bool
ClientContext::isRequestAddrConflict() const {
    return is_request_addr_conflict_;
}

IOAddress
ClientContext::getYourAddr() const {
    return your_addr_;
}

void
ClientContext::setYourAddr(IOAddress addr) {
    your_addr_ = addr;
}

std::string
ClientContext::getHostName() const {
    return "";
}

void 
ClientContext::setSharedSubnetID(uint32_t subnet_id) {
    shared_subnet_id_ = subnet_id;
}

};
};
