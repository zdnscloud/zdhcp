#pragma once

#include <stdexcept>
#include <string>
#include <sstream>
#include <kea/logging/logging.h>

using namespace kea::logging;

namespace kea {

class Exception : public std::exception {
public:
    Exception(const char* file, size_t line, const char* what);
    Exception(const char* file, size_t line, const std::string& what);
    virtual ~Exception() throw() {}

    void operator=(const Exception& src) = delete;

    virtual const char* what() const throw();
    virtual const char* what(bool verbose) const throw();
    const std::string& getMessage() const { return (what_); }
    const char* getFile() const { return (file_); }
    size_t getLine() const { return (line_); }

private:
    const char* const file_;
    size_t line_;
    const std::string what_;
    std::string verbose_what_;
};

class OutOfRange : public Exception {
public:
    OutOfRange(const char* file, size_t line, const char* what) :
        kea::Exception(file, line, what) {}
};

class InvalidParameter : public Exception {
public:
    InvalidParameter(const char* file, size_t line, const char* what) :
        kea::Exception(file, line, what) {}
};

class BadValue : public Exception {
public:
    BadValue(const char* file, size_t line, const char* what) :
        kea::Exception(file, line, what) {}
};

class InvalidOperation : public Exception {
public:
    InvalidOperation(const char* file, size_t line, const char* what) :
        kea::Exception(file, line, what) {}
};

class Unexpected : public Exception {
public:
    Unexpected(const char* file, size_t line, const char* what) :
        kea::Exception(file, line, what) {}
};

class NotImplemented : public Exception {
public:
    NotImplemented(const char* file, size_t line, const char* what) :
        kea::Exception(file, line, what) {}
};

class RFCViolation : public Exception {
public:
    RFCViolation(const char* file, size_t line, const char* what) :
        kea::Exception(file, line, what) {}
};

#define kea_throw(type, stream) \
    do { \
        std::ostringstream oss__; \
        oss__ << stream; \
        logError("Exceptions", oss__.str()); \
        throw type(__FILE__, __LINE__, oss__.str().c_str()); \
    } while (1)

#define kea_throw_1(type, stream, param1) \
    do { \
        std::ostringstream oss__; \
        oss__ << stream; \
        logError("Exceptions", oss__.str()); \
        throw type(__FILE__, __LINE__, oss__.str().c_str(), param1); \
    } while (1)

#define kea_throw_2(type, stream, param1, param2) \
    do { \
        std::ostringstream oss__; \
        oss__ << stream; \
        logError("Exceptions", oss__.str()); \
        throw type(__file__, __line__, oss__.str().c_str(), param1, param2); \
    } while (1)

#define kea_throw_3(type, stream, param1, param2, param3) \
    do { \
        std::ostringstream oss__; \
        oss__ << stream; \
        logError("Exceptions", oss__.str()); \
        throw type(__file__, __line__, oss__.str().c_str(), param1, param2,\
                   param3); \
    } while (1)

#define kea_throw_4(type, stream, param1, param2, param3, param4) \
    do { \
        std::ostringstream oss__; \
        oss__ << stream; \
        logError("Exceptions", oss__.str()); \
        throw type(__FILE__, __LINE__, oss__.str().c_str(), param1, param2,\
                   param3, param4); \
    } while (1)

};
