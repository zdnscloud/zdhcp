#pragma once

#include <map>
#include <string>
#include <memory>
#include <kea/exceptions/exceptions.h>

#define DHCP4_OPTION_SPACE "dhcp4"

namespace kea {
namespace dhcp {

class InvalidOptionSpace : public Exception {
public:
    InvalidOptionSpace(const char* file, size_t line, const char* what) :
        kea::Exception(file, line, what) { };
};

class OptionSpace;
typedef std::shared_ptr<OptionSpace> OptionSpacePtr;
typedef std::map<std::string, OptionSpacePtr> OptionSpaceCollection;

class OptionSpace {
public:
    OptionSpace(const std::string& name, const bool vendor_space = false);

    const std::string& getName() const { return (name_); }

    void clearVendorSpace() {
        vendor_space_ = false;
    }

    bool isVendorSpace() const { return (vendor_space_); }

    void setVendorSpace() {
        vendor_space_ = true;
    }

    static bool validateName(const std::string& name);

private:
    std::string name_;  ///< Holds option space name.
    bool vendor_space_; ///< Is this the vendor space?
};

};
};
