#include <kea/dhcp++/hwaddr.h>
#include <kea/exceptions/exceptions.h>
#include <kea/util/strutil.h>
#include <iomanip>
#include <sstream>
#include <vector>
#include <string>

namespace kea {
namespace dhcp {

const uint32_t HWAddr::HWADDR_SOURCE_ANY = 0xffffffff;
const uint32_t HWAddr::HWADDR_SOURCE_UNKNOWN = 0x00000000;
const uint32_t HWAddr::HWADDR_SOURCE_RAW = 0x00000001;
const uint32_t HWAddr::HWADDR_SOURCE_DUID = 0x00000002;
const uint32_t HWAddr::HWADDR_SOURCE_CLIENT_ADDR_RELAY_OPTION = 0x00000008;
const uint32_t HWAddr::HWADDR_SOURCE_REMOTE_ID = 0x00000010;
const uint32_t HWAddr::HWADDR_SOURCE_SUBSCRIBER_ID = 0x00000020;
const uint32_t HWAddr::HWADDR_SOURCE_DOCSIS_CMTS = 0x00000040;
const uint32_t HWAddr::HWADDR_SOURCE_DOCSIS_MODEM = 0x00000080;

HWAddr::HWAddr()
    :htype_(HTYPE_ETHER), source_(0) {
}

HWAddr::HWAddr(const HWAddr& addr)
    :hwaddr_(addr.hwaddr_), htype_(addr.htype_), source_(addr.source_) {
}

HWAddr& HWAddr::operator=(const HWAddr& addr) {
    if (this != &addr) {
        hwaddr_ = addr.hwaddr_;
        htype_ = addr.htype_;
        source_ = addr.source_;
    }
    return *this;
}

HWAddr::HWAddr(const uint8_t* hwaddr, size_t len, uint16_t htype)
    :hwaddr_(hwaddr, hwaddr + len), htype_(htype), source_(0) {
    if (len > MAX_HWADDR_LEN) {
        kea_throw(kea::BadValue, "hwaddr length exceeds MAX_HWADDR_LEN");
    }
}

HWAddr::HWAddr(const std::vector<uint8_t>& hwaddr, uint16_t htype)
    :hwaddr_(hwaddr), htype_(htype), source_(0) {
    if (hwaddr.size() > MAX_HWADDR_LEN) {
        kea_throw(kea::BadValue,
            "address vector size exceeds MAX_HWADDR_LEN");
    }
}

std::string HWAddr::toText(bool include_htype) const {
    std::stringstream tmp;
    if (include_htype) {
        tmp << "hwtype=" << static_cast<unsigned int>(htype_) << " ";
    }
    tmp << std::hex;
    bool delim = false;
    for (auto i8 : hwaddr_) {
        if (delim) {
            tmp << ":";
        }
        tmp << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(i8);
        delim = true;
    }
    return (tmp.str());
}

std::unique_ptr<HWAddr> HWAddr::fromText(const std::string& text, uint16_t htype) {
    std::vector<uint8_t> binary(MAX_HWADDR_LEN);
    kea::util::str::decodeColonSeparatedHexString(text, binary);
    return std::unique_ptr<HWAddr>(new HWAddr(binary, htype));
}

bool HWAddr::operator==(const HWAddr& other) const {
    return ((this->htype_  == other.htype_) &&
            (this->hwaddr_ == other.hwaddr_));
}

bool HWAddr::operator!=(const HWAddr& other) const {
    return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, const HWAddr& hwaddr) {
    os << hwaddr.toText();
    return (os);
}

}; 
};
