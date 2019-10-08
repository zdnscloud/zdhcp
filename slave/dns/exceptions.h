#pragma once
#include <kea/exceptions/exceptions.h>

namespace kea {
namespace dns {

class Exception : public kea::Exception {
public:
    Exception(const char* file, size_t line, const char* what) :
        kea::Exception(file, line, what) {}
};

class DNSTextError : public kea::dns::Exception {
public:
    DNSTextError(const char* file, size_t line, const char* what) :
        kea::dns::Exception(file, line, what) {}
};

class NameParserException : public DNSTextError {
public:
    NameParserException(const char* file, size_t line, const char* what) :
        DNSTextError(file, line, what) {}
};

class DNSProtocolError : public kea::dns::Exception {
    public:
            DNSProtocolError(const char* file, size_t line, const char* what) :
                        kea::dns::Exception(file, line, what) {}
};

class DNSMessageFORMERR : public DNSProtocolError {
public:
    DNSMessageFORMERR(const char* file, size_t line, const char* what) :
        DNSProtocolError(file, line, what) {}
};

};
};
