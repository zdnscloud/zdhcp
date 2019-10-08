#include <kea/dhcp++/dhcp4.h>
#include <kea/nic/iface_mgr.h>
#include <kea/nic/pkt_filter_inet.h>
#include <kea/exceptions/exceptions.h>

#include <cstring>
#include <errno.h>
#include <fstream>
#include <sstream>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/select.h>
#include <net/if.h>

using namespace std;
using namespace kea::util;

namespace {
    static kea::nic::IfaceMgr* SingletonIfaceMgr = nullptr;
}

namespace kea {
namespace nic {

void IfaceMgr::init() {
    if (SingletonIfaceMgr == nullptr) {
        SingletonIfaceMgr = new IfaceMgr();
        std::atexit([](){ delete SingletonIfaceMgr;});
    }
}

IfaceMgr& IfaceMgr::instance() {
    return *SingletonIfaceMgr;
}

IfaceMgr::IfaceMgr(): 
     packet_filter_(new PktFilterInet()),
     test_mode_(false)
{
    control_buf_.reserve(CMSG_SPACE(sizeof(struct in6_pktinfo)));

    try {
        detectIfaces();
    } catch (const std::exception& ex) {
        kea_throw(IfaceDetectError, ex.what());
    }
}

void IfaceMgr::closeSockets() {
    for (auto &iface : ifaces_) {
        iface->closeSockets();
    }
}

IfaceMgr::~IfaceMgr() {
    control_buf_.clear();
    closeSockets();
}

bool IfaceMgr::isDirectResponseSupported() const {
    return (packet_filter_->isDirectResponseSupported());
}

void IfaceMgr::setPacketFilter(std::unique_ptr<PktFilter> packet_filter) {
    if (!packet_filter) {
        kea_throw(InvalidPacketFilter, "NULL packet filter object specified for"
                " DHCPv4");
    }

    if (hasOpenSocket()) {
        kea_throw(PacketFilterChangeDenied,
                "it is not allowed to set new packet"
                << " filter when there are open IPv4 sockets - need"
                << " to close them first");
    }
    packet_filter_ = std::move(packet_filter);
}

bool IfaceMgr::hasOpenSocket() const {
    for(auto &iface : ifaces_) {
        if (iface->getSockets().empty() == false) {
            return (true);
        }
    }
    return (false);
}

bool IfaceMgr::hasOpenSocket(const IOAddress& addr) const {
    for(auto &iface : ifaces_) {
        for (auto &sock_info : iface->getSockets()) {
            if (sock_info.addr_ == addr){
                return (true);
            } else if (sock_info.addr_.isV4Zero()) {
                for (auto& iface_addr : iface->getAddresses()) {
                    if (iface_addr == addr) {
                        return (true);
                    }
                }
            }
        }
    }

    return (false);
}

bool IfaceMgr::openSockets4(uint16_t port, bool use_bcast, 
        IfaceMgrErrorMsgCallback error_handler) {
    int count = 0;
    int bcast_num = 0;

    for(auto &iface : ifaces_) {
        if (!iface->isActive()) {
            continue;
        } else {
            if (iface->isLoopBack()) {
                IFACEMGR_ERROR(SocketConfigError, error_handler,
                        "must not open socket on the loopback"
                        " interface " << iface->getName());
                continue;

            }

            IOAddress out_address("0.0.0.0");
            if (!iface->isUp()|| !iface->isRunning()|| !iface->getAddress(out_address)) {
                IFACEMGR_ERROR(SocketConfigError, error_handler,
                        "the interface " << iface->getName()
                        << " is down or has no usable IPv4"
                        " addresses configured");
                continue;
            }
        }

        for(auto& addr : iface->getAddresses()) {
            if (iface->isBroadcast() && use_bcast) {
                if (!isDirectResponseSupported() && bcast_num > 0) {
                    IFACEMGR_ERROR(SocketConfigError, error_handler,
                            "Binding socket to an interface is not"
                            " supported on this OS; therefore only"
                            " one socket listening to broadcast traffic"
                            " can be opened. Sockets will not be opened"
                            " on remaining interfaces");
                    continue;

                } else {
                    try {
                        openSocket(iface->getName(), addr, port, true, true);
                    } catch (const Exception& ex) {
                        IFACEMGR_ERROR(SocketConfigError, error_handler,
                                "failed to open socket on interface "
                                << iface->getName() << ", reason: "
                                << ex.what());
                        continue;

                    }
                    ++bcast_num;
                }
            } else {
                try {
                    openSocket(iface->getName(), addr, port, false, false);
                } catch (const Exception& ex) {
                    IFACEMGR_ERROR(SocketConfigError, error_handler,
                            "failed to open socket on interface "
                            << iface->getName() << ", reason: "
                            << ex.what());
                    continue;
                }

            }
            ++count;

        }
    }
    return (count > 0);
}

void IfaceMgr::printIfaces(std::ostream& out) {
    for (auto& iface : ifaces_) {
        const Iface::AddressCollection& addrs = iface->getAddresses();
        out << "Detected interface " << iface->getFullName()
            << ", hwtype=" << iface->getHWType()
            << ", mac=" << iface->getPlainMac();
        out << "("
            << (iface->isLoopBack() ? "LOOPBACK ":"")
            << (iface->isUp() ? "UP ":"")
            << (iface->isRunning() ? "RUNNING ":"")
            << (iface->isMulticast() ? "MULTICAST ":"")
            << (iface->isBroadcast() ? "BROADCAST ":"")
            << ")" << endl;
        out << "  " << addrs.size() << " addr(s):";

        for (auto& addr : addrs) {
            out << "  " << addr.toText();
        }
        out << endl;
    }
}

const Iface* IfaceMgr::getIface(int ifindex) {
    for (auto& iface : ifaces_) {
        if (iface->getIndex() == ifindex) {
            return (iface.get());
        }
    }
    return (nullptr);
}

const Iface* IfaceMgr::getIface(const std::string& ifname) {
    for (auto& iface : ifaces_) {
        if (iface->getName() == ifname)
            return (iface.get());
    }
    return (nullptr);
}

void IfaceMgr::clearIfaces() {
    ifaces_.clear();
}

void IfaceMgr::clearUnicasts() {
    for (auto& iface : ifaces_) {
        iface->clearUnicasts();
    }
}

int IfaceMgr::openSocket(const std::string& ifname, 
        const IOAddress& addr, uint16_t port,
        bool receive_bcast, bool send_bcast) {
    Iface* iface = const_cast<Iface *>(getIface(ifname));
    if (iface == nullptr) {
        kea_throw(BadValue, "There is no " << ifname << " interface present.");
    }

    return openSocket4(*iface, addr, port, receive_bcast, send_bcast);
}

int IfaceMgr::openSocketFromIface(const std::string& ifname,
        uint16_t port) {
    for (auto& iface : ifaces_) {
        if ((iface->getFullName() != ifname) &&
            (iface->getName() != ifname)) {
            continue;
        }

        Iface::AddressCollection addrs = iface->getAddresses();
        Iface::AddressCollection::iterator addr_it = addrs.begin();
        for(auto& addr : addrs) {
            return (openSocket(iface->getName(), addr, port, false));
        }
        kea_throw(SocketConfigError, "There is no address for interface: "
                  << ifname << ", port: " << port << ", address ");
    }
    kea_throw(BadValue, "There is no " << ifname << " interface present.");
}

int IfaceMgr::openSocketFromAddress(const IOAddress& addr,
        const uint16_t port) {
    for (auto& iface : ifaces_) {
        for (auto& addr_tmp : iface->getAddresses()) {
            if (addr_tmp == addr) {
                return (openSocket(iface->getName(), addr, port, false));
            }
        }
    }
    kea_throw(BadValue, "There is no such address " << addr);
}

int IfaceMgr::openSocketFromRemoteAddress(const IOAddress& remote_addr,
        uint16_t port) {
    try {
        IOAddress local_address(getLocalAddress(remote_addr, port));
        return openSocketFromAddress(local_address, port);
    } catch (const Exception& e) {
        kea_throw(SocketConfigError, e.what());
    }
}

IOAddress IfaceMgr::getLocalAddress(const IOAddress& remote_addr,
        uint16_t port) {
    return IOAddress("0.0.0.0");
}

int IfaceMgr::openSocket4(Iface& iface, const IOAddress& addr,
        uint16_t port, bool receive_bcast, bool send_bcast) {
    SocketInfo info = packet_filter_->openSocket(iface, addr, port,
                                                 receive_bcast, send_bcast);
    iface.addSocket(info);
    return (info.sockfd_);
}

bool IfaceMgr::send(Pkt& pkt) {
    const Iface *iface = getIface(pkt.getIface());
    if (iface == nullptr) {
        kea_throw(BadValue, "Unable to send DHCPv4 message. Invalid interface ("
                << pkt.getIface() << ") specified.");
    }
    return (packet_filter_->send(*(const_cast<Iface*>(iface)), getSocket(pkt).sockfd_, pkt));
}


std::unique_ptr<Pkt> IfaceMgr::receive4(int stop_fd, 
        uint32_t timeout_sec, uint32_t timeout_usec /* = 0 */) {
    if (timeout_usec >= 1000000) {
        kea_throw(BadValue, "fractional timeout must be shorter than"
                  " one million microseconds");
    }
    fd_set sockets;
    int maxfd = 0;
    FD_ZERO(&sockets);
    for (auto& iface : ifaces_) {
        for (auto &sock_info : iface->getSockets()) {
            FD_SET(sock_info.sockfd_, &sockets);
            if (maxfd < sock_info.sockfd_) {
                maxfd = sock_info.sockfd_;
            }
        }
    }

    FD_SET(stop_fd, &sockets);
    if (maxfd < stop_fd) {
        maxfd = stop_fd;
    }

    struct timeval select_timeout;
    select_timeout.tv_sec = timeout_sec;
    select_timeout.tv_usec = timeout_usec;
    errno = 0;

    int result = select(maxfd + 1, &sockets, NULL, NULL, &select_timeout);

    if (result == 0) {
        return (std::unique_ptr<Pkt>());
    } else if (result < 0) {
        if (errno == EINTR) {
            kea_throw(SignalInterruptOnSelect, strerror(errno));
        } else {
            kea_throw(SocketReadError, strerror(errno));
        }
    }

    if (FD_ISSET(stop_fd, &sockets)) {
        return (std::unique_ptr<Pkt>());
    }

    const SocketInfo *candidate = nullptr;
    Iface *iface_to_send = nullptr;
    for (auto& iface : ifaces_) {
        for (auto& sock_info : iface->getSockets()) {
            if (FD_ISSET(sock_info.sockfd_, &sockets)) {
                candidate = &(sock_info);
                iface_to_send = iface.get();
                break;
            }
        }
        if (candidate != nullptr) {
            break;
        }
    }

    if (candidate == nullptr) {
        kea_throw(SocketReadError, "received data over unknown socket");
    }

    return (packet_filter_->receive(*iface_to_send, *candidate));
}


SocketInfo IfaceMgr::getSocket(const Pkt& pkt) {
    const Iface* iface = getIface(pkt.getIface());
    if (iface == nullptr) {
        kea_throw(IfaceNotFound, "Tried to find socket for non-existent interface");
    }

    const SocketInfo* candidate = nullptr;
    for (auto& sock_info : iface->getSockets()){
        if (sock_info.addr_ == pkt.getLocalAddr()) {
            return (sock_info);
        }

        if (candidate == nullptr) {
            candidate = &(sock_info);
        }
    }

    if (candidate == nullptr) {
        kea_throw(SocketNotFound, "Interface " << iface->getFullName()
                << " does not have any suitable IPv4 sockets open.");
    }

    return (*candidate);
}

}; 
};
