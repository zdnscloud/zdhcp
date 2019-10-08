#pragma once

#include <kea/dhcp++/subnet.h>
#include <kea/server/subnet_selector.h>

namespace kea {
namespace server {

using kea::dhcp::Subnet;
using kea::dhcp::SubnetID;
using kea::dhcp::Subnet4Collection;

class SubnetMgr{
public:
    typedef std::unique_ptr<Subnet> Subnet4Ptr;

    bool add(Subnet4Ptr);

    const Subnet* selectSubnet(const SubnetSelector&) const;
    const Subnet* selectSubnet(const IOAddress&) const;
    const Subnet*  getSubnet(SubnetID id) const;
    const Subnet* selectSubnet(const IOAddress&, const ClientClasses&) const;

private:
    bool isDuplicate(const Subnet& subnet) const;
    const Subnet* selectSubnet(const std::string&, const ClientClasses&) const;

    Subnet4Collection subnets_;
};

};
};
