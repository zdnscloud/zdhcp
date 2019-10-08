#pragma once

#include <stdint.h>
#include <string>
#include <vector>

namespace isc {
namespace util {
namespace encode {

std::string encodeBase32Hex(const std::vector<uint8_t>& binary);

void decodeBase32Hex(const std::string& input, std::vector<uint8_t>& result);

}; 
}; 
};
