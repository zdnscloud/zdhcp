#pragma once

#include <unistd.h>             
#include <stdint.h>             
#include <asio/ip/address.hpp>

#include <functional>
#include <string>
#include <vector>

#include <kea/exceptions/exceptions.h>

namespace kea {
namespace util {

    const static size_t V4ADDRESS_LEN = 4;

class IOAddress {
public:
    IOAddress(const std::string& address_str);

    IOAddress(const asio::ip::address& asio_address);

    explicit IOAddress(uint32_t v4address);

    std::string toText() const;

    short getFamily() const;

    bool isV4() const {
        return (asio_address_.is_v4());
    }
    asio::ip::address_v4 toV4() const{
        return asio_address_.to_v4();
    }

    static IOAddress fromLong(uint32_t addr) {
        return  IOAddress(ntohl(addr));
    }

    static uint32_t toLong(IOAddress ipaddr) {
        auto str = ipaddr.toText();
        in_addr addr;
        assert(inet_pton(AF_INET, str.c_str(), &addr) == 1);
        return addr.s_addr;
    }

    bool isV4Zero() const {
        return (equals(IPV4_ZERO_ADDRESS()));
    }

    bool isV4Bcast() const {
        return (equals(IPV4_BCAST_ADDRESS()));
    }

    static IOAddress fromBytes(short family, const uint8_t* data);

    std::vector<uint8_t> toBytes() const;

    bool equals(const IOAddress& other) const {
        return (asio_address_ == other.asio_address_);
    }

    bool operator==(const IOAddress& other) const {
        return equals(other);
    }

    bool nequals(const IOAddress& other) const {
        return (!equals(other));
    }

    bool lessThan(const IOAddress& other) const {
        if (this->getFamily() == other.getFamily()) {
            if (this->getFamily() == AF_INET) {
                return (this->asio_address_.to_v4() < other.asio_address_.to_v4());
            }
        }
        return (this->getFamily() < other.getFamily());
    }

    bool smallerEqual(const IOAddress& other) const {
        if (equals(other)) {
            return (true);
        }
        return (lessThan(other));
    }

    bool operator<(const IOAddress& other) const {
        return (lessThan(other));
    }

    bool operator<=(const IOAddress& other) const {
        return (smallerEqual(other));
    }

    bool operator!=(const IOAddress& other) const {
        return (nequals(other));
    }

    static IOAddress subtract(const IOAddress& a, const IOAddress& b);

    static IOAddress
    increase(const IOAddress& addr);

    operator uint32_t () const;

    static const IOAddress& IPV4_ZERO_ADDRESS() {
        static IOAddress address(0);
        return (address);
    }

    static const IOAddress& IPV4_BCAST_ADDRESS() {
        static IOAddress address(0xFFFFFFFF);
        return (address);
    }


private:
    asio::ip::address asio_address_;
};

std::ostream&
operator<<(std::ostream& os, const IOAddress& address);

} 
}
