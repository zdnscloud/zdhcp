#include <kea/dhcp++/option4_addrlst.h>
#include <kea/exceptions/exceptions.h>
#include <kea/util/io_utilities.h>

#include <iomanip>
#include <sstream>

#include <arpa/inet.h>
#include <cstdint>
#include <cstring>

using namespace std;
using namespace kea::util;

namespace kea {
namespace dhcp {

Option4AddrLst::Option4AddrLst(uint8_t type)
    :Option(type) {
}

Option4AddrLst::Option4AddrLst(uint8_t type, const AddressContainer& addrs)
    :Option(type) {
    setAddresses(addrs);
}


Option4AddrLst::Option4AddrLst(uint8_t type, OptionBufferConstIter first,
        OptionBufferConstIter last)
    :Option(type) {
    if (distance(first, last) % V4ADDRESS_LEN != 0) {
        kea_throw(OutOfRange, "DHCPv4 Option4AddrLst " << type_
                  << " has invalid length=" << distance(first, last)
                  << ", must be divisible by 4.");
    }

    while (first != last) {
        const uint8_t* ptr = &(*first);
        addAddress(IOAddress(readUint32(ptr, distance(first, last))));
        first += V4ADDRESS_LEN;
    }
}

Option4AddrLst::Option4AddrLst(uint8_t type, const IOAddress& addr)
    :Option(type) {
    setAddress(addr);
}

std::unique_ptr<Option> Option4AddrLst::clone() const {
    return (cloneInternal<Option4AddrLst>());
}

void Option4AddrLst::pack(kea::util::OutputBuffer& buf) const {
    if (addrs_.size() * V4ADDRESS_LEN > 255) {
        kea_throw(OutOfRange, "DHCPv4 Option4AddrLst " << type_ << " is too big."
                  << "At most 255 bytes are supported.");
        /// TODO Larger options can be stored as separate instances
        /// of DHCPv4 options. Clients MUST concatenate them.
        /// Fortunately, there are no such large options used today.
    }

    buf.writeUint8(type_);
    buf.writeUint8(len() - getHeaderLen());

    for (auto& addr : addrs_) {
        buf.writeUint32(addr);
    }
}

void Option4AddrLst::setAddress(const IOAddress& addr) {
    if (!addr.isV4()) {
        kea_throw(BadValue, "Can't store non-IPv4 address in "
                  << "Option4AddrLst option");
    }
    addrs_.clear();
    addAddress(addr);
}

void Option4AddrLst::setAddresses(const AddressContainer& addrs) {
    addrs_.clear();
    for (auto& addr : addrs) {
        addAddress(addr);
    }
}

void Option4AddrLst::addAddress(const IOAddress& addr) {
    if (!addr.isV4()) {
        kea_throw(BadValue, "Can't store non-IPv4 address in "
                  << "Option4AddrLst option");
    }
    addrs_.push_back(addr);
}

uint16_t Option4AddrLst::len() const {
    return (getHeaderLen() + addrs_.size() * V4ADDRESS_LEN);
}

std::string Option4AddrLst::toText(int indent) const {
    std::stringstream output;
    output << headerToText(indent) << ":";

    for (auto& addr : addrs_) {
        output << " " << addr; 
    }
    return (output.str());
}

std::string Option4AddrLst::toString() const {
    std::stringstream output;
    int addr_len = addrs_.size();
    for (int i = 0; i < addr_len; i++) {
        output << addrs_[i]; 
        if (i + 1 != addr_len) {
            output << ",";
        }
    }
    return output.str();
}

}; 
};
