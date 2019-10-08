#include <kea/server/host.h>
#include <kea/util/encode/hex.h>
#include <kea/util/strutil.h>
#include <kea/exceptions/exceptions.h>
#include <sstream>

namespace kea {
namespace server {

Host::Host(const uint8_t* identifier, const size_t identifier_len,
        const IdentifierType& identifier_type, SubnetID ipv4_subnet_id, 
        const IOAddress& ipv4_reservation, const std::string& hostname,
        const std::string& dhcp4_client_classes)
    : ipv4_subnet_id_(ipv4_subnet_id), 
      ipv4_reservation_(IOAddress("0.0.0.0")),
      hostname_(hostname), 
      dhcp4_client_classes_(dhcp4_client_classes),
      host_id_(0){
    setIdentifier(identifier, identifier_len, identifier_type);

    if (!ipv4_reservation.isV4Zero()) {
        setIPv4Reservation(ipv4_reservation);
    }
}

Host::Host(const std::string& identifier, const std::string& identifier_name,
           SubnetID ipv4_subnet_id, const IOAddress& ipv4_reservation,
           const std::string& hostname, const std::string& dhcp4_client_classes)
    : ipv4_subnet_id_(ipv4_subnet_id),
      ipv4_reservation_(IOAddress("0.0.0.0")),
      hostname_(hostname), 
      dhcp4_client_classes_(dhcp4_client_classes),
      host_id_(0){

    // Initialize host identifier.
    setIdentifier(identifier, identifier_name);
    if (!ipv4_reservation.isV4Zero()) {
        setIPv4Reservation(ipv4_reservation);
    }
}

const std::vector<uint8_t>& Host::getIdentifier() const {
    if (hwaddr_) {
        return (hwaddr_->hwaddr_);
    } else if (duid_) {
        return (duid_->getDuid());
    }
    static std::vector<uint8_t> empty_vector;
    return (empty_vector);
}

Host::IdentifierType Host::getIdentifierType() const {
    if (hwaddr_) {
        return (IDENT_HWADDR);
    }
    return (IDENT_DUID);
}

Host::IdentifierType Host::getIdentifierType(const std::string& identifier_name) {
    if (identifier_name == "hw-address") {
        return (IDENT_HWADDR);

    } else if (identifier_name == "duid") {
        return (IDENT_DUID);

    } else if (identifier_name == "circuit-id") {
        return (IDENT_CIRCUIT_ID);

    } else if (identifier_name == "client-id") {
        return (IDENT_CLIENT_ID);

    } else {
        kea_throw(kea::BadValue, "invalid client identifier type '"
                  << identifier_name << "'");
    }
}

std::string Host::getIdentifierAsText() const {
    std::ostringstream s;
    if (hwaddr_) {
        s << "hwaddr=" << hwaddr_->toText(false);
    } else {
        s << "duid=";
        if (duid_) {
            s << duid_->toText();
        } else {
            s << "(none)";
        }
    }

    return (s.str());
}

void Host::setIdentifier(const uint8_t* identifier, const size_t len,
        const IdentifierType& type) {
    switch (type) {
        case IDENT_HWADDR:
            hwaddr_ = std::unique_ptr<HWAddr>(new HWAddr(identifier, len, HTYPE_ETHER));
            duid_.reset();
            break;
        case IDENT_DUID:
            duid_ = std::unique_ptr<DUID>(new DUID(identifier, len));
            hwaddr_.reset();
            break;
        default:
            kea_throw(BadValue, "invalid client identifier type '"
                    << static_cast<int>(type) << "' when creating host "
                    " instance");
    }   
}

void Host::setIdentifier(const std::string& identifier, const std::string& name) {
    if (name == "hw-address") {
        hwaddr_ = HWAddr::fromText(identifier);
        duid_.reset();
    } else if (name == "duid") {
        duid_ = std::unique_ptr<DUID>(new DUID(DUID::fromText(identifier)));
        hwaddr_.reset();
    } else {
        kea_throw(BadValue, "invalid client identifier type '"
                << name << "' when creating host instance");
    }
}

void Host::setIPv4Reservation(const IOAddress& address) {
    if (!address.isV4()) {
        kea_throw(kea::BadValue, "address '" << address << "' is not a valid"
                  " IPv4 address");
    } else if (address.isV4Zero() || address.isV4Bcast()) {
        kea_throw(kea::BadValue, "must not make reservation for the '"
                  << address << "' address");
    }
    ipv4_reservation_ = address;
}

void Host::removeIPv4Reservation() {
    ipv4_reservation_ = IOAddress("0.0.0.0");
}

void Host::addClientClass4(const std::string& class_name) {
    addClientClassInternal(dhcp4_client_classes_, class_name);
}

void Host::addClientClassInternal(ClientClasses& classes,
        const std::string& class_name) {
    std::string trimmed = util::str::trim(class_name);
    if (!trimmed.empty()) {
        classes.insert(ClientClass(trimmed));
    }
}

std::string Host::toText() const {
    std::ostringstream s;
    // Add HW address or DUID.
    s << getIdentifierAsText();

    // Add IPv4 subnet id if exists (non-zero).
    if (ipv4_subnet_id_) {
        s << " ipv4_subnet_id=" << ipv4_subnet_id_;
    }

    // Add hostname.
    s << " hostname=" << (hostname_.empty() ? "(empty)" : hostname_);

    // Add IPv4 reservation.
    s << " ipv4_reservation=" << (ipv4_reservation_.isV4Zero() ? "(no)" :
                                  ipv4_reservation_.toText());

    // Add DHCPv4 client classes.
    for (ClientClasses::const_iterator cclass = dhcp4_client_classes_.begin();
         cclass != dhcp4_client_classes_.end(); ++cclass) {
        s << " dhcp4_class"
          << std::distance(dhcp4_client_classes_.begin(), cclass)
          << "=" << *cclass;
    }
    return (s.str());
}

}; 
};
