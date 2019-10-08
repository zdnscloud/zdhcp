#pragma once

#include <kea/dhcp++/option.h>
#include <kea/util/buffer.h>

#include <string>

namespace kea {
namespace dhcp {

class OptionString : public Option {
public:

    OptionString(const uint16_t type, const std::string& value);

    OptionString(const uint16_t type,
                 OptionBufferConstIter begin, OptionBufferConstIter end);

    std::unique_ptr<Option> clone() const;

    virtual uint16_t len() const;

    std::string getValue() const;

    void setValue(const std::string& value);

    virtual void pack(kea::util::OutputBuffer& buf) const;

    virtual void unpack(OptionBufferConstIter begin, OptionBufferConstIter end);

    virtual std::string toText(int indent = 0) const;

    virtual std::string toString() const;
};

typedef std::shared_ptr<OptionString> OptionStringPtr;

}; 
};
