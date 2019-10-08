#include <kea/util/io_address.h>
#include <kea/exceptions/exceptions.h>
#include <kea/logging/logging.h>

#include <unistd.h>          
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <asio/ip/udp.hpp>
#include <asio/ip/tcp.hpp>

using namespace asio;
using asio::ip::udp;
using asio::ip::tcp;

using namespace kea::logging;

using namespace std;

namespace kea {
namespace util {

IOAddress::IOAddress(const std::string& address_str) {
    std::error_code err;
    asio_address_ = ip::address::from_string(address_str, err);
    if (err) {
        logError("IOAddress ", "Failed to convert string \"$0\" to address (error: $1)", address_str, err.message());
    }
}

IOAddress::IOAddress(const asio::ip::address& asio_address) :
    asio_address_(asio_address)
{}

IOAddress::IOAddress(uint32_t v4address):
    asio_address_(asio::ip::address_v4(v4address)) {

}

string
IOAddress::toText() const {
    return (asio_address_.to_string());
}

IOAddress
IOAddress::fromBytes(short family, const uint8_t* data) {
    assert(data != NULL); 
    assert(family == AF_INET); 

    if (family == AF_INET) {
        char addr_str[INET_ADDRSTRLEN];
        inet_ntop(family, data, addr_str, INET_ADDRSTRLEN);
        return IOAddress(string(addr_str));
    }
}

std::vector<uint8_t>
IOAddress::toBytes() const {
    if (asio_address_.is_v4()) {
        const asio::ip::address_v4::bytes_type bytes4 =
            asio_address_.to_v4().to_bytes();
        return (std::vector<uint8_t>(bytes4.begin(), bytes4.end()));
    }
}

short
IOAddress::getFamily() const {
    if (asio_address_.is_v4()) {
        return (AF_INET);
    }
}

IOAddress::operator uint32_t() const {
    assert(asio_address_.is_v4());
    return (asio_address_.to_v4().to_ulong());
}

std::ostream&
operator<<(std::ostream& os, const IOAddress& address) {
    os << address.toText();
    return (os);
}

IOAddress
IOAddress::subtract(const IOAddress& a, const IOAddress& b) {
    assert(a.getFamily() != b.getFamily());
    if (a.isV4()) {
        return (IOAddress(static_cast<uint32_t>(a) - static_cast<uint32_t>(b)));
    } 
}

IOAddress
IOAddress::increase(const IOAddress& addr) {
    std::vector<uint8_t> packed(addr.toBytes());

    for (int i = packed.size() - 1; i >= 0; --i) {
        if (++packed[i] != 0) {
            break;
        }
    }

    return (IOAddress::fromBytes(addr.getFamily(), &packed[0]));
}


} 
}
