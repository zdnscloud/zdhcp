#include <kea/util/ipaddress_extend.h>

namespace kea {
namespace util {

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

uint64_t addrCountInRange(const IOAddress& first, const IOAddress& last) {
    assert(last >= first);

    return (last - first + 1);
}

IOAddress lastAddrInNetwork(const IOAddress& prefix, uint8_t prefix_len) {
    assert((prefix_len != 0) && (prefix_len <= 32));

    uint32_t addr = prefix;
    return (IOAddress(addr | bitMask4[prefix_len]));
}

IOAddress firstAddrInNetwork(const IOAddress& prefix, uint8_t len) {
    assert(len <= 32);

    uint32_t addr = prefix;
    return (IOAddress(addr & (~bitMask4[len])));
}

IOAddress getNetmask4(uint8_t len) {
    assert(len <= 32); 
    uint32_t x = ~bitMask4[len];

    return (IOAddress(x));
}

};
};
