#pragma once
#include <kea/server/client_class_matcher.h>

namespace kea {
namespace server {

class StringLiteralFormatError : public Exception {
    public:
        StringLiteralFormatError(const char* file, size_t line, const char* what) :
            kea::Exception(file, line, what) { };
};

PktOptionMatcher buildMatcher(const std::string& expression);

};
};

