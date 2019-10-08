#pragma once

#include <kea/dhcp++/dhcp4.h>
#include <kea/dhcp++/pkt.h>
#include <kea/nic/pkt_filter.h>
#include <kea/nic/iface.h>

#include <vector>

using namespace kea::dhcp;

namespace kea {
namespace nic {

typedef std::function<void(const std::string&)> IfaceMgrErrorMsgCallback;
class IfaceMgr {
public:
    static const uint32_t RCVBUFSIZE = 1500;
    static void init();
    static IfaceMgr& instance();

    typedef std::vector<std::unique_ptr<Iface>> IfaceCollection;

    IfaceMgr();
    IfaceMgr(const IfaceMgr&) = delete;
    IfaceMgr& operator=(const IfaceMgr&) = delete;

    virtual ~IfaceMgr();
    void setTestMode(const bool test_mode) {
        test_mode_ = test_mode;
    }
    bool isTestMode() const {
        return (test_mode_);
    }

    bool isDirectResponseSupported() const;

    const Iface* getIface(int ifindex);
    const Iface* getIface(const std::string& ifname);

    const IfaceCollection& getIfaces() { return (ifaces_); }

    void clearIfaces();
    void detectIfaces();
    void clearUnicasts();

    SocketInfo getSocket(const Pkt& pkt);

    void printIfaces(std::ostream& out = std::cout);

    bool send(Pkt& pkt);

    std::unique_ptr<Pkt> receive4(int stop_fd, uint32_t timeout_sec, uint32_t timeout_usec = 0);

    int openSocket(const std::string& ifname, const IOAddress& addr,
            const uint16_t port, const bool receive_bcast = false, const bool send_bcast = false);

    int openSocketFromIface(const std::string& ifname, uint16_t port);

    int openSocketFromAddress(const IOAddress& addr, const uint16_t port);

    int openSocketFromRemoteAddress(const IOAddress& remote_addr, const uint16_t port);

    bool openSockets4(uint16_t port = DHCP4_SERVER_PORT,
            bool use_bcast = true, IfaceMgrErrorMsgCallback error_handler = NULL);

    void closeSockets();
    uint16_t countIfaces() { return ifaces_.size(); }

    void setPacketFilter(std::unique_ptr<PktFilter> packet_filter);
    void setMatchingPacketFilter(const bool direct_response_desired = false);

    void addInterface(std::unique_ptr<Iface> iface) {
        ifaces_.push_back(std::move(iface));
    }

    bool hasOpenSocket(const IOAddress& addr) const;
    bool hasOpenSocket() const;

protected:
    int openSocket4(Iface& iface, const IOAddress& addr, uint16_t port, 
            bool receive_bcast = false, bool send_bcast = false);

    void stubDetectIfaces();

    IOAddress getLocalAddress(const IOAddress& remote_addr, const uint16_t port);

    bool openMulticastSocket(Iface& iface, const IOAddress& addr, uint16_t port,
            IfaceMgrErrorMsgCallback error_handler = NULL);

    IfaceCollection ifaces_;
    std::vector<uint8_t> control_buf_;
    std::unique_ptr<PktFilter> packet_filter_;
    bool test_mode_;
};

#define IFACEMGR_ERROR(ex_type, handler, stream) \
{ \
    std::ostringstream ieoss__; \
    ieoss__ << stream; \
    if (handler) { \
        handler(ieoss__.str()); \
    } else { \
        kea_throw(ex_type, ieoss__.str()); \
    } \
} 
}; 
};
