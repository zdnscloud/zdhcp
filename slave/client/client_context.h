#pragma once

#include <kea/dhcp++/subnet.h>
#include <kea/dhcp++/pkt.h>
#include <kea/util/io_address.h>

namespace kea {
namespace client {


using kea::util::IOAddress;
using kea::dhcp::PktPtr;
using kea::dhcp::Pkt;
using kea::dhcp::Subnet;

class ClientContext {
public:
    ClientContext(PktPtr query, const Subnet& subnet);

    std::vector<uint8_t> getClientID() const;
    std::vector<uint8_t> getHWAddr() const; 
    IOAddress getRequestAddr() const; 

    Pkt& getQuery() { return *query_; }
    uint8_t getQueryType() const; 

    const Subnet& getSubnet() { return subnet_; }

    uint32_t getSubnetID() const  { return subnet_.getID(); }

    uint32_t getSharedSubnetID() const  { return shared_subnet_id_; }
    void setSharedSubnetID(uint32_t subnet_id); 

    void setRequestAddrConflict(bool conflict); 
    bool isRequestAddrConflict() const; 

    IOAddress getYourAddr() const;
    void setYourAddr(IOAddress addr);

    std::string getHostName() const;
    
    void addRetryCount() { retry_count_ += 1;}
    int getRetryCount() const { return retry_count_; }

private:
    const Subnet& subnet_;
    uint32_t shared_subnet_id_;
    PktPtr query_;
    bool is_request_addr_conflict_;
    IOAddress your_addr_;
    int retry_count_;
};

typedef std::unique_ptr<ClientContext> ClientContextPtr;
typedef std::function<void(ClientContextPtr)> ClientContextHandler;

};
};

