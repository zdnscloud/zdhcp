#include <sstream>
#include <kea/dhcp++/option_string.h>

namespace kea {
namespace dhcp {

OptionString::OptionString(const uint16_t type,
        const std::string& value)
    : Option(type) {
    setValue(value);
}

OptionString::OptionString(const uint16_t type,
        OptionBufferConstIter begin, OptionBufferConstIter end)
    : Option(type) {
    unpack(begin, end);
}

std::unique_ptr<Option> OptionString::clone() const {
    return (cloneInternal<OptionString>());
}

std::string OptionString::getValue() const {
    const OptionBuffer& data = getData();
    return (std::string(data.begin(), data.end()));
}

void OptionString::setValue(const std::string& value) {
    if (value.empty()) {
        kea_throw(kea::OutOfRange, "string value carried by the option '"
                  << getType() << "' must not be empty");
    }

    setData(value.begin(), value.end());
}

uint16_t OptionString::len() const {
    return (getHeaderLen() + getData().size());
}

void OptionString::pack(kea::util::OutputBuffer& buf) const {
    packHeader(buf);
    const OptionBuffer& data = getData();
    buf.writeData(&data[0], data.size());
}

void OptionString::unpack(OptionBufferConstIter begin,
                     OptionBufferConstIter end) {
    if (std::distance(begin, end) == 0) {
        kea_throw(kea::OutOfRange, "failed to parse an option '"
                  << getType() << "' holding string value"
                  << " - empty value is not accepted");
    }
    setData(begin, end);
}

std::string OptionString::toText(int indent) const {
    std::ostringstream output;
    output << headerToText(indent) << ": "
           << "\"" << getValue() << "\" (string)";

    return (output.str());
}

std::string OptionString::toString() const {
    return (getValue());
}

};
};
