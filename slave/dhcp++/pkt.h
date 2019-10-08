#pragma once

#include <kea/dhcp++/option.h>
#include <kea/dhcp++/duid.h>
#include <kea/dhcp++/option.h>
#include <kea/dhcp++/classify.h>
#include <kea/dhcp++/pkt.h>
#include <kea/util/buffer.h>
#include <kea/dhcp++/hwaddr.h>
#include <kea/util/io_address.h>

#include <set>
#include <vector>
#include <chrono>
#include <iostream>

using namespace kea::util;

namespace kea {
namespace dhcp {

typedef std::chrono::system_clock::time_point TimePoint;

class Pkt {
public:
    const static size_t MAX_CHADDR_LEN = 16;
    const static size_t MAX_SNAME_LEN = 64;
    const static size_t MAX_FILE_LEN = 128;
    const static size_t DHCPV4_PKT_HDR_LEN = 236;
    const static uint16_t FLAG_BROADCAST_MASK = 0x8000;

    Pkt(const uint8_t* data, size_t len);
    Pkt(uint8_t msg_type, uint32_t transid);

    void pack();
    void unpack();
    
    void addOption(const std::unique_ptr<Option>);
    bool delOption(uint16_t type);
    const Option* getOption(uint16_t type) const;

    void setTransid(uint32_t transid) { transid_ = transid; }
    uint32_t getTransid() const { return (transid_); };

    void addClass(const ClientClass& client_class);
    const ClientClasses& getClasses() const { return (classes_); }

    void updateTimestamp() { timestamp_ = std::chrono::system_clock::now(); }
    kea::util::OutputBuffer& getBuffer() { return (buffer_out_); };

    std::string getLabel() const;
    static std::string makeLabel(const HWAddr* hwaddr, const ClientId* client_id,
        uint32_t transid);
    static std::string makeLabel(const HWAddr* hwaddr, const ClientId* client_id);

    std::string toText() const;
    size_t len() const;
    
    void setLocalAddr(const IOAddress& local) { local_addr_ = local; }
    const IOAddress& getLocalAddr() const { return (local_addr_); }

    void setRemoteAddr(const IOAddress& remote) { remote_addr_ = remote; }
    const IOAddress& getRemoteAddr() const { return (remote_addr_); }
    
    void setLocalPort(uint16_t local) { local_port_ = local; }
    uint16_t getLocalPort() const { return (local_port_); }

    void setRemotePort(uint16_t remote) { remote_port_ = remote; }
    uint16_t getRemotePort() const { return (remote_port_); }

    void setIfaceIndex(uint32_t ifindex) { ifindex_ = ifindex; };
    uint32_t getIfaceIndex() const { return (ifindex_); };

    std::string getIface() const { return (iface_); };
    void setIface(const std::string& iface ) { iface_ = iface; };

    void setRemoteHWAddr(const uint8_t htype, const uint8_t hlen,
            const std::vector<uint8_t>& hw_addr) {
        remote_hwaddr_ = HWAddr(hw_addr, htype);
    }

    void setRemoteHWAddr(const HWAddr& hw_addr) { remote_hwaddr_ = hw_addr; }
    const HWAddr& getRemoteHWAddr() const { return (remote_hwaddr_); }

    void setHops(uint8_t hops) { hops_ = hops; };
    uint8_t getHops() const { return (hops_); };

    uint8_t getOp() const { return (op_); };
    void setSecs(uint16_t secs) { secs_ = secs; };
    uint16_t getSecs() const { return (secs_); };

    void setFlags(uint16_t flags) { flags_ = flags; };
    uint16_t getFlags() const { return (flags_); };

    const IOAddress& getCiaddr() const { return (ciaddr_); };
    void setCiaddr(const IOAddress& ciaddr) { ciaddr_ = ciaddr; };

    const IOAddress& getSiaddr() const { return (siaddr_); };
    void setSiaddr(const IOAddress& siaddr) { siaddr_ = siaddr; };

    const IOAddress& getYiaddr() const { return (yiaddr_); };
    void setYiaddr(const IOAddress& yiaddr) { yiaddr_ = yiaddr; };

    const IOAddress& getGiaddr() const { return (giaddr_); };
    void setGiaddr(const IOAddress& giaddr) { giaddr_ = giaddr; };

    uint8_t getType() const;
    void setType(uint8_t type);

    static const char* getName(const uint8_t type);
    const char* getName() const;

    const OptionBuffer getSname() const { 
        return (std::vector<uint8_t>(sname_, &sname_[MAX_SNAME_LEN])); };
    void setSname(const uint8_t* sname, size_t sname_len = MAX_SNAME_LEN);

    const OptionBuffer getFile() const { 
        return (std::vector<uint8_t>(file_, &file_[MAX_FILE_LEN])); };
    void setFile(const uint8_t* file, size_t file_len = MAX_FILE_LEN);

    void setHWAddr(const HWAddr& addr) { hwaddr_ = addr; }
    void setHWAddr(uint8_t htype, uint8_t hlen, const std::vector<uint8_t>& mac_addr);
    const HWAddr& getHWAddr() const { return (hwaddr_); }

    uint8_t getHlen() const {
        uint8_t len = hwaddr_.hwaddr_.size();
        return (len <= MAX_CHADDR_LEN ? len : MAX_CHADDR_LEN);
    }
    uint8_t getHtype() const { return (hwaddr_.htype_); }

    void setLocalHWAddr(const HWAddr& addr) { local_hwaddr_ = addr; }
    void setLocalHWAddr(const uint8_t htype, const uint8_t hlen,
                        const std::vector<uint8_t>& mac_addr);

    const HWAddr& getLocalHWAddr() const { return (local_hwaddr_); }
    bool isRelayed() const;

private:
    uint8_t DHCPTypeToBootpType(uint8_t dhcpType);

    int ifindex_;
    uint32_t transid_;
    std::string iface_;

    IOAddress local_addr_;
    IOAddress remote_addr_;

    TimePoint timestamp_;
    util::OutputBuffer buffer_out_;
    bool copy_retrieved_options_;
    
    HWAddr hwaddr_;
    HWAddr local_hwaddr_;
    HWAddr remote_hwaddr_;

    OptionBuffer data_;
    ClientClasses classes_;
    OptionCollection options_;

    uint8_t op_;
    uint8_t hops_;
    uint16_t secs_;
    uint16_t flags_;
    uint16_t local_port_;
    uint16_t remote_port_;
    
    IOAddress ciaddr_;
    IOAddress yiaddr_;
    IOAddress siaddr_;
    IOAddress giaddr_;
    uint8_t sname_[MAX_SNAME_LEN];
    uint8_t file_[MAX_FILE_LEN];
}; 

typedef std::unique_ptr<Pkt> PktPtr;
};
};
