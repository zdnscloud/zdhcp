#include <kea/dhcp++/dhcp4.h>
#include <kea/dhcp++/libdhcp++.h>
#include <kea/dhcp++/option_vendor.h>
#include <sstream>

namespace kea {
namespace dhcp {

OptionVendor::OptionVendor(const uint32_t vendor_id)
    :Option(DHO_VIVSO_SUBOPTIONS), vendor_id_(vendor_id) {
}

OptionVendor::OptionVendor(OptionBufferConstIter begin,
                           OptionBufferConstIter end)
    :Option(DHO_VIVSO_SUBOPTIONS), vendor_id_(0) {
    unpack(begin, end);
}

std::unique_ptr<Option>
OptionVendor::clone() const {
    return (cloneInternal<OptionVendor>());
}

void OptionVendor::pack(kea::util::OutputBuffer& buf) const {
    packHeader(buf);
    buf.writeUint32(vendor_id_);
    buf.writeUint8(dataLen());
    packOptions(buf);
}

void OptionVendor::unpack(OptionBufferConstIter begin,
        OptionBufferConstIter end) {
    if (distance(begin, end) < sizeof(uint32_t)) {
        kea_throw(OutOfRange, "Truncated vendor-specific information option"
                  << ", length=" << distance(begin, end));
    }

    vendor_id_ = kea::util::readUint32(&(*begin), distance(begin, end));
    OptionBuffer vendor_buffer(begin + 4, end);
    LibDHCP::unpackVendorOptions4(vendor_id_, vendor_buffer, options_);
}

uint16_t OptionVendor::len() const {
    uint16_t length = getHeaderLen();
    length += sizeof(uint32_t); 
    length += sizeof(uint8_t); 

    for (auto& pair : options_) {
        length += pair.second->len();
    }
    return (length);
}

uint8_t OptionVendor::dataLen() const {
    return (len() - getHeaderLen() - sizeof(uint32_t) - sizeof(uint8_t));
}

std::string OptionVendor::toText(int indent) const {
    std::stringstream output;
    output << headerToText(indent) << ": "
           << getVendorId() << " (uint32)";
    output << " " << static_cast<int>(dataLen()) << " (uint8)";
    output << suboptionsToText(indent + 2);

    return (output.str());
}

};
};
