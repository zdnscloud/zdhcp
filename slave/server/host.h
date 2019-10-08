#pragma once
#include <kea/dhcp++/classify.h>
#include <kea/dhcp++/duid.h>
#include <kea/dhcp++/hwaddr.h>
#include <kea/server/subnet_id.h>
#include <kea/util/io_address.h>

namespace kea {
namespace server {

typedef uint64_t HostID;
using namespace kea::util;
using namespace kea::dhcp;

class Host {
public:
    enum IdentifierType {
        IDENT_HWADDR,
        IDENT_DUID,
        IDENT_CIRCUIT_ID,
        IDENT_CLIENT_ID
    };

    static const IdentifierType LAST_IDENTIFIER_TYPE = IDENT_CLIENT_ID;

    Host(const uint8_t* identifier, const size_t identifier_len,
            const IdentifierType& identifier_type, SubnetID ipv4_subnet_id,
            const IOAddress& ipv4_reservation, const std::string& hostname = "",
            const std::string& dhcp4_client_classes = "");

    Host(const std::string& identifier, const std::string& identifier_name,
            SubnetID ipv4_subnet_id, const IOAddress& ipv4_reservation,
            const std::string& hostname = "", const std::string& dhcp4_client_classes = "");

    void setIdentifier(const uint8_t* identifier, const size_t len,
            const IdentifierType& type);

    void setIdentifier(const std::string& identifier, const std::string& name);

    const HWAddr* getHWAddress() const { return hwaddr_.get(); };

    const DUID* getDuid() const { return duid_.get(); };

    const std::vector<uint8_t>& getIdentifier() const;

    IdentifierType getIdentifierType() const;

    static IdentifierType getIdentifierType(const std::string& identifier_name);

    std::string getIdentifierAsText() const;

    void setIPv4SubnetID(const SubnetID ipv4_subnet_id) {
        ipv4_subnet_id_ = ipv4_subnet_id;
    }

    SubnetID getIPv4SubnetID() const {
        return (ipv4_subnet_id_);
    }

    void setIPv4Reservation(const IOAddress& address);

    void removeIPv4Reservation();

    const IOAddress& getIPv4Reservation() const {
        return (ipv4_reservation_);
    }

    void setHostname(const std::string& hostname) {
        hostname_ = hostname;
    }

    const std::string& getHostname() const {
        return (hostname_);
    }

    void addClientClass4(const std::string& class_name);

    const ClientClasses& getClientClasses4() const {
        return (dhcp4_client_classes_);
    }

    std::string toText() const;

    void setHostId(HostID id) {
        host_id_ = id;
    }

    HostID getHostId() const {
        return (host_id_);
    }

private:
    void addClientClassInternal(ClientClasses& classes,
            const std::string& class_name);

    SubnetID ipv4_subnet_id_;
    IOAddress ipv4_reservation_;
    std::string hostname_;
    ClientClasses dhcp4_client_classes_;
    uint64_t host_id_;
    std::unique_ptr<HWAddr> hwaddr_;
    std::unique_ptr<DUID> duid_;
};

typedef std::vector<const Host*> HostCollection;

};
};
