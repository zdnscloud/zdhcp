#pragma once

#include <kea/exceptions/exceptions.h>
#include <cstdint>

namespace kea {
namespace server {
typedef uint32_t SubnetID;

class DuplicateSubnetID : public Exception {
public:
    DuplicateSubnetID(const char* file, size_t line, const char* what) :
        kea::Exception(file, line, what) { };
};

};
};
