#include <string>
#include <sstream>
#include <kea/exceptions/exceptions.h>

namespace kea {

Exception::Exception(const char* file, size_t line, const char* what)
: file_(file), line_(line), what_(what) {
    std::stringstream location;
    location << what_ << "[" << file_ << ":" << line_ << "]";
    verbose_what_ = location.str();
}

Exception::Exception(const char* file, size_t line, const std::string& what)
    : file_(file), line_(line), what_(what) {
    std::stringstream location;
    location << what_ << "[" << file_ << ":" << line_ << "]";
    verbose_what_ = location.str();
}

const char*
Exception::what() const throw() {
    return (what(false));
}

const char*
Exception::what(bool verbose) const throw() {
    try {
        if (verbose) {
            return (verbose_what_.c_str());
        } else {
            return (what_.c_str());
        }
    } catch (...) {
    }
    return ("kea::Exception");
}
};
