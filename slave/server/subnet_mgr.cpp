#include <kea/nic/iface_mgr.h>
#include <kea/server/subnet_mgr.h>
#include <kea/server/subnet_id.h>
#include <algorithm>

using namespace kea::nic;
namespace kea {
namespace server {

bool
SubnetMgr::add(Subnet4Ptr subnet) {
    if (isDuplicate(*subnet)) {
        logError("SubnetMgr ", "ID of the new IPv4 subnet $0 is already in use", subnet->getID());
        return false;
    }else {
        subnets_.push_back(std::move(subnet));
        return true;
    }
}

const Subnet*
SubnetMgr::selectSubnet(const SubnetSelector& selector) const {
    // First use RAI link select sub-option or subnet select option
    if (!selector.option_select_.isV4Zero()) {
        return (selectSubnet(selector.option_select_,
                             selector.client_classes_));
    }

    // If relayed message has been received, try to match the giaddr with the
    // relay address specified for a subnet. It is also possible that the relay
    // address will not match with any of the relay addresses accross all
    // subnets, but we need to verify that for all subnets before we can try
    // to use the giaddr to match with the subnet prefix.
    if (!selector.giaddr_.isV4Zero()) {
        for(auto& subnet : subnets_) {
            // Check if the giaddr is equal to the one defined for the subnet.
            if (selector.giaddr_ != subnet->getRelayInfo().addr_) {
                continue;
            }

            // If a subnet meets the client class criteria return it.
            if (subnet->clientSupported(selector.client_classes_)) {
                return (subnet.get());
            }
        }
    }

    IOAddress address= IOAddress("0.0.0.0");
    if (!selector.giaddr_.isV4Zero()) {
        address = selector.giaddr_;
    // If it is a Renew or Rebind, use the ciaddr.
    } else if (!selector.ciaddr_.isV4Zero() && !selector.local_address_.isV4Bcast()) {
        address = selector.ciaddr_;
    } else if (!selector.remote_address_.isV4Zero() && !selector.local_address_.isV4Bcast()) {
        address = selector.remote_address_;

    // If local interface name is known, use the local address on this
    // interface.
    } else if (!selector.iface_name_.empty()) {
        const Iface* iface = IfaceMgr::instance().getIface(selector.iface_name_);
        if (iface == NULL) {
            kea_throw(BadValue, "interface " << selector.iface_name_
                      << " doesn't exist and therefore it is impossible"
                      " to find a suitable subnet for its IPv4 address");
        }

        const Subnet* subnet = selectSubnet(selector.iface_name_,
                                         selector.client_classes_);
        if (subnet) {
            return (subnet);
        } else {
            IOAddress v4Addr(0);
            iface->getAddress(v4Addr);
            address = v4Addr;
        }
    }

    // Unable to find a suitable address to use for subnet selection.
    if (address.isV4Zero()) {
        return (nullptr);
    }

    // We have identified an address in the client's packet that can be
    // used for subnet selection. Match this packet with the subnets.
    return (selectSubnet(address, selector.client_classes_));
}

const Subnet* 
SubnetMgr::selectSubnet(const std::string& iface, const ClientClasses& client_classes) const {
    for(auto& subnet : subnets_) {
        // If there's no interface specified for this subnet, proceed to
        // the next subnet.
        if (subnet->getIface() != iface) {
            continue;
        }

        // If a subnet meets the client class criteria return it.
        if (subnet->clientSupported(client_classes)) {
            return (subnet.get());
        }
    }

    return (nullptr);
}

const Subnet*
SubnetMgr::selectSubnet(const IOAddress& address) const {
    return selectSubnet(address, ClientClasses());
}

const Subnet*
SubnetMgr::selectSubnet(const IOAddress& address, const ClientClasses& client_classes) const {
    for(auto& subnet : subnets_) {
        if (!subnet->inRange(address)) {
            continue;
        }

        if (subnet->clientSupported(client_classes)) {
            return (subnet.get());
        }
    }

    return (nullptr);
}

bool 
SubnetMgr::isDuplicate(const Subnet& subnet) const {
    for(auto& subnet_ : subnets_) {
        if (subnet_->getID() == subnet.getID()) {
            return (true);
        }
    }
    return (false);
}

const Subnet*
SubnetMgr::getSubnet(SubnetID id) const {
    for (auto& subnet_ : subnets_) {
        if (subnet_->getID() == id) {
            return (subnet_.get());
        }
    }
    logWarning("SubnetMgr ", "ID of subnet $0 is no exist", id);
    return nullptr;
}

};
};
