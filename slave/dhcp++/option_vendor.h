#pragma once

#include <kea/dhcp++/option.h>
#include <kea/dhcp++/option_data_types.h>
#include <kea/util/io_utilities.h>

#include <cstdint>

namespace kea {
namespace dhcp {

class OptionVendor: public Option {
public:
    OptionVendor(const std::uint32_t vendor_id);

    OptionVendor(OptionBufferConstIter begin,
                 OptionBufferConstIter end);

    std::unique_ptr<Option> clone() const;

    virtual void pack(kea::util::OutputBuffer& buf) const;

    virtual void unpack(OptionBufferConstIter begin, OptionBufferConstIter end);

    void setVendorId(const uint32_t vendor_id) { vendor_id_ = vendor_id; }

    uint32_t getVendorId() const { return (vendor_id_); }

    virtual uint16_t len() const;

    virtual std::string toText(int indent = 0) const;

private:
    uint8_t dataLen() const;

    uint32_t vendor_id_;  
};

typedef std::shared_ptr<OptionVendor> OptionVendorPtr;

}; 
};
