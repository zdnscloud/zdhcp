#include <kea/exceptions/exceptions.h>
#include <kea/dhcp++/opaque_data_tuple.h>
#include <kea/dhcp++/option_vendor_class.h>
#include <sstream>

namespace kea {
namespace dhcp {

OptionVendorClass::OptionVendorClass(const uint32_t vendor_id)
    : Option(DHO_VIVCO_SUBOPTIONS), vendor_id_(vendor_id) {
      addTuple(OpaqueDataTuple());
}

OptionVendorClass::OptionVendorClass(OptionBufferConstIter begin,
                                     OptionBufferConstIter end)
    : Option(DHO_VIVCO_SUBOPTIONS) {
    unpack(begin, end);
}

std::unique_ptr<Option> OptionVendorClass::clone() const {
    return (cloneInternal<OptionVendorClass>());
}

void OptionVendorClass::pack(kea::util::OutputBuffer& buf) const {
    packHeader(buf);
    buf.writeUint32(getVendorId());

    for (auto it = tuples_.begin(); it != tuples_.end(); ++it) {
        if (it != tuples_.begin()) {
            buf.writeUint32(getVendorId());
        }
        it->pack(buf);
    }
}

void OptionVendorClass::unpack(OptionBufferConstIter begin,
        OptionBufferConstIter end) {
    if (std::distance(begin, end) < MinimalLength - getHeaderLen()) {
        kea_throw(OutOfRange, "parsed Vendor Class option data truncated to"
                  " size " << std::distance(begin, end));
    }

    vendor_id_ = kea::util::readUint32(&(*begin), distance(begin, end));
    begin += sizeof(vendor_id_);

    size_t offset = 0;
    while (offset < std::distance(begin, end)) {
        OpaqueDataTuple tuple(begin + offset, end);
        addTuple(tuple);
        offset += tuple.getTotalLength();
        if (begin + offset != end) {
            offset += sizeof(vendor_id_);
            if (offset + 1 >= std::distance(begin, end)) {
                kea_throw(kea::OutOfRange, "truncated DHCPv4 V-I Vendor Class"
                          " option - it should contain enterprise id followed"
                          " by opaque data field tuple");
            }
        }
    }
}

void OptionVendorClass::addTuple(const OpaqueDataTuple& tuple) {
    tuples_.push_back(tuple);
}


void OptionVendorClass::setTuple(const size_t at, const OpaqueDataTuple& tuple) {
    if (at >= getTuplesNum()) {
        kea_throw(kea::OutOfRange, "attempted to set an opaque data for the"
                  " vendor option at position " << at << " which is out of"
                  " range");
    }

    tuples_[at] = tuple;
}

OpaqueDataTuple OptionVendorClass::getTuple(const size_t at) const {
    if (at >= getTuplesNum()) {
        kea_throw(kea::OutOfRange, "attempted to get an opaque data for the"
                  " vendor option at position " << at << " which is out of"
                  " range. There are only " << getTuplesNum() << " tuples");
    }
    return (tuples_[at]);
}

bool OptionVendorClass::hasTuple(const std::string& tuple_str) const {
    for (auto& tuple : tuples_) {
        if (tuple == tuple_str) {
            return (true);
        }
    }
    return (false);
}

uint16_t OptionVendorClass::len() const {
    uint16_t length = getHeaderLen() + sizeof(uint32_t);
    for (auto it = tuples_.begin(); it != tuples_.end(); ++it) {
        if (it != tuples_.begin()) {
            length += sizeof(uint32_t);
        }
        length += it->getTotalLength();
    }

    return (length);
}

std::string OptionVendorClass::toText(int indent) const {
    std::ostringstream s;

    s << std::string(indent, ' ');
    s << "type=" << getType() << ", len=" << len() - getHeaderLen() << ", "
        " enterprise id=0x" << std::hex << getVendorId() << std::dec;
    for (unsigned i = 0; i < getTuplesNum(); ++i) {
        if (i > 0) {
            s << ", enterprise id=0x" << std::hex << getVendorId() << std::dec;
        }

        auto tuple = getTuple(i);
        s << ", data-len" << i << "=" << tuple.getLength();
        s << ", vendor-class-data" << i << "='" << tuple << "'";
    }

    return (s.str());
}

};
};
