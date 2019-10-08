#pragma once
#include <kea/dhcp++/dhcp4.h>
#include <kea/dhcp++/pkt.h>
#include <kea/nic/pkt_filter.h>
#include <kea/util/io_address.h>

#include <vector>

using namespace kea::dhcp;
using namespace kea::util;

namespace kea {
namespace nic {

class IfaceDetectError : public Exception {
public:
    IfaceDetectError(const char* file, size_t line, const char* what) :
        Exception(file, line, what) { };
};

class PacketFilterChangeDenied : public Exception {
public:
    PacketFilterChangeDenied(const char* file, size_t line, const char* what) :
        Exception(file, line, what) { };
};

class SignalInterruptOnSelect : public Exception {
public:
    SignalInterruptOnSelect(const char* file, size_t line, const char* what) :
        Exception(file, line, what) { };
};

class SocketReadError : public Exception {
public:
    SocketReadError(const char* file, size_t line, const char* what) :
        Exception(file, line, what) { };
};

class SocketWriteError : public Exception {
public:
    SocketWriteError(const char* file, size_t line, const char* what) :
        Exception(file, line, what) { };
};

class IfaceNotFound : public Exception {
public:
    IfaceNotFound(const char* file, size_t line, const char* what) :
        Exception(file, line, what) { };
};

class SocketNotFound : public Exception {
public:
    SocketNotFound(const char* file, size_t line, const char* what) :
        Exception(file, line, what) { };
};

struct SocketInfo {
    IOAddress addr_; 
    uint16_t port_;
    int sockfd_;
    int fallbackfd_;

    SocketInfo(const IOAddress& addr, const uint16_t port, const int sockfd, const int fallbackfd = -1)
        : addr_(addr), port_(port), sockfd_(sockfd), fallbackfd_(fallbackfd) {}

};

typedef std::function<bool(const SocketInfo&)> SocketFilter;

class Iface {
public:
    static const unsigned int MAX_MAC_LEN = 20;

    static const uint16_t IFACE_LOOP_BACK = 0x01;
    static const uint16_t IFACE_UP        = 0x02;
    static const uint16_t IFACE_RUNNING   = 0x08;
    static const uint16_t IFACE_MULTI_CAST = 0x10;
    static const uint16_t IFACE_BOARD_CAST= 0x20;

    typedef std::vector<IOAddress> AddressCollection;
    typedef std::vector<SocketInfo> SocketCollection;

    Iface(const std::string& name, int ifindex);
    ~Iface() { }

    bool closeSockets();
    bool closeSockets(SocketFilter);

    std::string getFullName() const;
    std::string getPlainMac() const;

    void setMac(const uint8_t* mac, size_t macLen);
    size_t getMacLen() const { return mac_len_; }
    const uint8_t* getMac() const { return mac_; }

    void setFlags(uint64_t flags) { flags_ = flags;}
    bool isLoopBack() const; 
    bool isUp() const ;
    bool isRunning() const ;
    bool isMulticast() const ;
    bool isBroadcast() const ;
    bool isActive() const { return (inactive4_ == false); }
    void setActive(bool active) { inactive4_ = active; }

    uint16_t getIndex() const { return ifindex_; }

    std::string getName() const { return name_; };

    void setHWType(uint16_t type ) { hardware_type_ = type; }
    uint16_t getHWType() const { return hardware_type_; }

    const AddressCollection& getAddresses() const { return addrs_; }
    bool getAddress(IOAddress& addr) const;
    bool hasAddress(const IOAddress& addr) const;
    void addAddress(const IOAddress& addr);
    bool delAddress(const IOAddress& addr);

    void addSocket(const SocketInfo& sock) {
        sockets_.push_back(sock);
    }
    bool delSocket(uint16_t sockfd);

    const SocketCollection& getSockets() const { return sockets_; }

    void clearUnicasts() { unicasts_.clear(); }
    void addUnicast(const IOAddress& addr);

    const AddressCollection& getUnicasts() const {
        return unicasts_;
    }

    uint8_t* getReadBuffer() {
        if (read_buffer_.empty()) {
            return NULL;
        }
        return (&read_buffer_[0]);
    }

    size_t getReadBufferSize() const {
        return (read_buffer_.size());
    }

    void resizeReadBuffer(const size_t new_size) {
        read_buffer_.resize(new_size);
    }

protected:
    Iface(const Iface&) = delete;
    Iface& operator=(const Iface&) = delete;

    SocketCollection sockets_;
    std::string name_;
    int ifindex_;
    AddressCollection addrs_;
    AddressCollection unicasts_;
    uint8_t mac_[MAX_MAC_LEN];
    size_t mac_len_;
    uint16_t hardware_type_;
    uint64_t flags_;
    bool inactive4_;

private:
    std::vector<uint8_t> read_buffer_;
};
};
};
