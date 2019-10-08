#pragma once

#include <stdint.h>
#include <string>
#include <vector>


namespace kea {
namespace util {
namespace encode {

std::string encodeBase64(const std::vector<uint8_t>& binary);

void decodeBase64(const std::string& input, std::vector<uint8_t>& result);

};
};
};
