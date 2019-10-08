#pragma once

#include <vector>
#include <stdint.h>
#include <stddef.h>
#include <string>
#include <memory>

#include <kea/dhcp++/dhcp4.h>

namespace kea {
namespace dhcp {
    
struct HWAddr {
public:
    static const size_t ETHERNET_HWADDR_LEN = 6;
    static const size_t MAX_HWADDR_LEN = 20;

    static const uint32_t HWADDR_SOURCE_ANY;
    static const uint32_t HWADDR_SOURCE_UNKNOWN;
    static const uint32_t HWADDR_SOURCE_RAW;
    static const uint32_t HWADDR_SOURCE_DUID;
    static const uint32_t HWADDR_SOURCE_CLIENT_ADDR_RELAY_OPTION;
    static const uint32_t HWADDR_SOURCE_REMOTE_ID;
    static const uint32_t HWADDR_SOURCE_SUBSCRIBER_ID;
    static const uint32_t HWADDR_SOURCE_DOCSIS_CMTS;
    static const uint32_t HWADDR_SOURCE_DOCSIS_MODEM;


    HWAddr();
    HWAddr(const HWAddr&);
    HWAddr& operator=(const HWAddr&);

    HWAddr(const uint8_t* hwaddr, size_t len, uint16_t htype);
    HWAddr(const std::vector<uint8_t>& hwaddr, uint16_t htype);

    std::string toText(bool include_htype = true) const;
    static std::unique_ptr<HWAddr> fromText(const std::string& text, uint16_t htype = HTYPE_ETHER);

    bool operator==(const HWAddr& other) const;
    bool operator!=(const HWAddr& other) const;

    std::vector<uint8_t> hwaddr_;
    uint16_t htype_;
    uint32_t source_;
};

std::ostream& operator<<(std::ostream& os, const HWAddr& addr); 
};
};
