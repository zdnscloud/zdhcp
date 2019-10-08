#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <cstdint>
#include <unistd.h>

namespace kea {
namespace dhcp {

class DUID {
 public:
    static const size_t MAX_DUID_LEN = 128;
    static const size_t MIN_DUID_LEN = 1;

    typedef enum {
        DUID_UNKNOWN = 0, ///< invalid/unknown type
        DUID_LLT = 1,     ///< link-layer + time, see RFC3315, section 9.2
        DUID_EN = 2,      ///< enterprise-id, see RFC3315, section 9.3
        DUID_LL = 3,      ///< link-layer, see RFC3315, section 9.4
        DUID_UUID = 4,    ///< UUID, see RFC6355
        DUID_MAX          ///< not a real type, just maximum defined value + 1
    } DUIDType;

    DUID(const std::vector<uint8_t>& duid);
    DUID(const uint8_t* duid, size_t len);

    const std::vector<uint8_t>& getDuid() const;
    static std::unique_ptr<DUID> generateEmpty();

    DUIDType getType() const;

    static DUID fromText(const std::string& text);
    std::string toText() const;

    bool operator==(const DUID& other) const;
    bool operator!=(const DUID& other) const;

 protected:
    std::vector<uint8_t> duid_;
};

class ClientId : public DUID {
public:

    static const size_t MIN_CLIENT_ID_LEN = 2;

    static const size_t MAX_CLIENT_ID_LEN = DUID::MAX_DUID_LEN;

    ClientId(const std::vector<uint8_t>& clientid);
    ClientId(const uint8_t* clientid, size_t len);

    const std::vector<uint8_t>& getClientId() const;

    std::string toText() const;
    static std::unique_ptr<ClientId> fromText(const std::string& text);

    bool operator==(const ClientId& other) const;
    bool operator!=(const ClientId& other) const;
};

}; 
};
