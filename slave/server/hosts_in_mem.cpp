#include <kea/server/hosts_in_mem.h>
#include <kea/exceptions/exceptions.h>
#include <ostream>

namespace kea {
namespace server {

HostCollection HostsInMem::getAll(const HWAddr* hwaddr, const DUID* duid) const {
    HostCollection hosts;
    for (auto& host : hosts_) {
        const HWAddr* hhwaddr = host->getHWAddress();
        const DUID* hduid = host->getDuid();
        if (hwaddr != nullptr && hhwaddr != nullptr && *hhwaddr == *hwaddr) {
           hosts.push_back(host.get());
        } else if (duid != nullptr && hduid != nullptr && *hduid == *duid) {
           hosts.push_back(host.get());
        } 
    }
    return std::move(hosts);
}

HostCollection HostsInMem::getAll4(const IOAddress& address) const {
    if (!address.isV4()) {
        kea_throw(BadHostAddress, "must specify an IPv4 address when searching"
                  " for a host, specified address was " << address.toText());
    }

    HostCollection hosts;
    for (auto& host : hosts_) {
        if (host->getIPv4Reservation() == address) {
            hosts.push_back(host.get());
        }
    }
    return std::move(hosts);
}

const Host* HostsInMem::get4(SubnetID subnet_id, const IOAddress& address) const {
    for (auto& host : hosts_) {
        if (host->getIPv4SubnetID() == subnet_id && host->getIPv4Reservation() == address) {
            return (host.get());
        }
    }
    return (nullptr);
}

const Host* HostsInMem::get4(SubnetID subnet_id, Host::IdentifierType type, 
        const uint8_t* identifier, size_t len) const {
    for (auto& host : hosts_) {
        if (host->getIPv4SubnetID() != subnet_id) {
            continue;
        }
        
        if (host->getIdentifierType() != type) {
            continue;
        }

        const std::vector<uint8_t> identifierVec = host->getIdentifier();
        if (len != identifierVec.size()) {
            continue;
        }

        if (memcmp(identifier, &identifierVec[0], len) == 0) {
            return (host.get());
        }
    }
    return (nullptr);
}

const Host* HostsInMem::get4(SubnetID subnet_id, const HWAddr* hwaddr, 
        const DUID* duid) const {
    for (auto& host : hosts_) {
        if (subnet_id != host->getIPv4SubnetID()) {
            continue;
        }

        const HWAddr *hhwaddr = host->getHWAddress();
        if (hwaddr != nullptr && hhwaddr != nullptr && *hhwaddr == *hwaddr) {
            return (host.get());
        } 
        
        const DUID *hduid = host->getDuid();
        if (duid != nullptr && hduid != nullptr && *hduid == *duid) {
            return (host.get());
        } 
    }
    return (nullptr);
}


void HostsInMem::add(std::unique_ptr<Host> host) {
    if (host->getIPv4SubnetID() == 0) {
        kea_throw(BadValue, "must not use both IPv4 and IPv6 subnet ids of"
                " 0 when adding new host reservation");
    }

    const HWAddr *hwaddr = host->getHWAddress();
    const DUID   *duid = host->getDuid();

    if (host->getHostname().empty() &&
            (host->getIPv4Reservation().isV4Zero())){
        std::ostringstream s;
        if (hwaddr) {
            s << "for DUID: " << hwaddr->toText();
        } else if (duid) {
            s << "for HW address: " << duid->toText();
        }   
        kea_throw(BadValue, "specified reservation " << s.str()
                << " must include at least one resource, i.e. "
                "hostname, IPv4 address or IPv6 address/prefix");
    }

    if ((host->getIPv4SubnetID() > 0) &&
            get4(host->getIPv4SubnetID(), hwaddr, duid) != nullptr) {
        kea_throw(DuplicateHost, "failed to add new host using the HW"
                " address '" << (hwaddr ? hwaddr->toText(false) : "(null)")
                << " and DUID '" << (duid ? duid->toText() : "(null)")
                << "' to the IPv4 subnet id '" << host->getIPv4SubnetID()
                << "' as this host has already been added");
    }

    if (!host->getIPv4Reservation().isV4Zero() &&
            (host->getIPv4SubnetID() > 0) &&
            get4(host->getIPv4SubnetID(), host->getIPv4Reservation()) != nullptr) {
        kea_throw(ReservedAddress, "failed to add new host using the HW"
                " address '" << (hwaddr ? hwaddr->toText(false) : "(null)")
                << " and DUID '" << (duid ? duid->toText() : "(null)")
                << "' to the IPv4 subnet id '" << host->getIPv4SubnetID()
                << "' for the address " << host->getIPv4Reservation()
                << ": There's already a reservation for this address");
    }

    hosts_.push_back(std::move(host));
}

};
};
