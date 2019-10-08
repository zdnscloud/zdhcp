#include <kea/dhcp++/pool.h>
#include <kea/util/ipaddress_extend.h>
#include <kea/exceptions/exceptions.h>
#include <sstream>

namespace kea {
namespace dhcp {

const uint32_t bitMask4[] = { 
    0xffffffff, 0x7fffffff, 0x3fffffff, 0x1fffffff,
    0x0fffffff, 0x07ffffff, 0x03ffffff, 0x01ffffff,
    0x00ffffff, 0x007fffff, 0x003fffff, 0x001fffff,
    0x000fffff, 0x0007ffff, 0x0003ffff, 0x0001ffff,
    0x0000ffff, 0x00007fff, 0x00003fff, 0x00001fff,
    0x00000fff, 0x000007ff, 0x000003ff, 0x000001ff,
    0x000000ff, 0x0000007f, 0x0000003f, 0x0000001f,
    0x0000000f, 0x00000007, 0x00000003, 0x00000001,
    0x00000000 
};

bool Pool::inRange(const IOAddress& addr) const {
    return ((first_ < addr || first_ == addr) && (addr < last_ || addr == last_));
}

Pool::Pool(const IOAddress& first, const IOAddress& last)
    : id_(getNextID()), first_(first), last_(last), capacity_(0) {
    if (!first.isV4() || !last.isV4()) {
        kea_throw(BadValue, "Invalid Pool4 address boundaries: not IPv4");
    }

    if (last < first) {
        kea_throw(BadValue, "Upper boundary is smaller than lower boundary.");
    }
    capacity_ = addrCountInRange(first, last); 
}

Pool::Pool( const IOAddress& prefix, uint8_t prefix_len)
    : id_(getNextID()), first_(prefix) 
    , last_(IOAddress("0.0.0.0")), capacity_(0) {
    if (!prefix.isV4()) {
        kea_throw(BadValue, "Invalid Pool4 address boundaries: not IPv4");
    }

    if (prefix_len == 0 || prefix_len > 32) {
        kea_throw(BadValue, "Invalid prefix length");
    }

    last_ = lastAddrInNetwork(first_, prefix_len);
    capacity_ = addrCountInRange(first_, last_);
}

std::string Pool::toText() const {
    std::stringstream tmp;
    tmp << first_.toText() << "-" << last_.toText();
    return (tmp.str());
}

};
};
