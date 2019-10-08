#pragma once

#include <kea/util/io_address.h>

namespace kea {
namespace util {

uint64_t addrCountInRange(const IOAddress& first, const IOAddress& last);
IOAddress lastAddrInNetwork(const IOAddress& prefix, uint8_t prefix_len);
IOAddress firstAddrInNetwork(const IOAddress& prefix, uint8_t len);
IOAddress getNetmask4(uint8_t len);

};
};
