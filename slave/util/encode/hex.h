#pragma once

#include <stdint.h>
#include <string>
#include <vector>

namespace kea {
namespace util {
namespace encode {

std::string encodeHex(const std::vector<uint8_t>& binary);

void decodeHex(const std::string& input, std::vector<uint8_t>& result);

};
};
};
