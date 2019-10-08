#include <kea/dhcp++/duid.h>
#include <kea/exceptions/exceptions.h>
#include <kea/util/io_utilities.h>

#include <kea/util/strutil.h>
#include <iomanip>
#include <cctype>
#include <sstream>
#include <vector>
#include <cstdint>

namespace kea {
namespace dhcp {

DUID::DUID(const std::vector<std::uint8_t>& duid) {
    if (duid.size() > MAX_DUID_LEN) {
        kea_throw(BadValue, "DUID too large");
    }
    if (duid.empty()) {
        kea_throw(BadValue, "Empty DUIDs are not allowed");
    }
    duid_ = duid;
}

DUID::DUID(const uint8_t* data, size_t len) {
    if (len > MAX_DUID_LEN) {
        kea_throw(BadValue, "DUID too large");
    }
    if (len == 0) {
        kea_throw(BadValue, "Empty DUIDs/Client-ids not allowed");
    }

    duid_ = std::vector<uint8_t>(data, data + len);
}

const std::vector<uint8_t>& DUID::getDuid() const {
    return (duid_);
}

DUID::DUIDType DUID::getType() const {
    if (duid_.size() < 2) {
        return (DUID_UNKNOWN);
    }
    uint16_t type = (duid_[0] << 8) + duid_[1];
    if (type < DUID_MAX) {
        return (static_cast<DUID::DUIDType>(type));
    } else {
        return (DUID_UNKNOWN);
    }
}

DUID DUID::fromText(const std::string& text) {
    std::vector<uint8_t> binary;
    util::str::decodeFormattedHexString(text, binary);
    return (DUID(binary));
}

std::unique_ptr<DUID> DUID::generateEmpty() {
    // Technically this is a one byte DUID with value of 0. However, we do have
    // a number of safety checks against invalid duids (too long or empty) and
    // we should keep them. Therefore "empty" means a single byte with value of 0.
    std::vector<uint8_t> empty_duid(1,0);
    return (std::unique_ptr<DUID>(new DUID(empty_duid)));
}

std::string DUID::toText() const {
    std::stringstream tmp;
    tmp << std::hex;
    bool delim = false;
    for (std::vector<uint8_t>::const_iterator it = duid_.begin();
         it != duid_.end(); ++it) {
        if (delim) {
            tmp << ":";
        }
        tmp << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(*it);
        delim = true;
    }
    return (tmp.str());
}

bool DUID::operator==(const DUID& other) const {
    return (this->duid_ == other.duid_);
}

bool DUID::operator!=(const DUID& other) const {
    return (this->duid_ != other.duid_);
}

ClientId::ClientId(const std::vector<uint8_t>& clientid)
    : DUID(clientid) {
    if (clientid.size() < MIN_CLIENT_ID_LEN) {
        kea_throw(BadValue, "client-id is too short (" << clientid.size()
                  << "), at least 2 is required");
    }
}

ClientId::ClientId(const uint8_t *clientid, size_t len)
    : DUID(clientid, len) {
    if (len < MIN_CLIENT_ID_LEN) {
        kea_throw(BadValue, "client-id is too short (" << len
                  << "), at least 2 is required");
    }
}

const std::vector<uint8_t>& ClientId::getClientId() const {
    return (duid_);
}

std::string ClientId::toText() const {
    // As DUID is a private base class of ClientId, we can't access
    // its public toText() method through inheritance: instead we
    // need the interface of a ClientId::toText() that calls the
    // equivalent method in the base class.
    return (DUID::toText());
}

std::unique_ptr<ClientId> ClientId::fromText(const std::string& text) {
    std::vector<uint8_t> binary;
    util::str::decodeFormattedHexString(text, binary);
    return (std::unique_ptr<ClientId>(new ClientId(binary)));
}

bool ClientId::operator==(const ClientId& other) const {
    return (this->duid_ == other.duid_);
}

bool ClientId::operator!=(const ClientId& other) const {
    return (this->duid_ != other.duid_);
}

}; 
};
