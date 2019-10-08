#include <kea/nic/iface.h>
#include <kea/nic/pkt_filter_inet.h>
#include <kea/exceptions/exceptions.h>
#include <net/if.h>

#include <algorithm>
#include <arpa/inet.h>
#include <netinet/in.h>

using namespace std;
using namespace kea::util;
using namespace kea::dhcp;

namespace kea {
namespace nic {

Iface::Iface(const std::string& name, int ifindex)
    :name_(name), ifindex_(ifindex), mac_len_(0), hardware_type_(0),
     flags_(0), inactive4_(false) {
    memset(mac_, 0, sizeof(mac_));
}

bool Iface::closeSockets() {
    return (closeSockets([](const SocketInfo &s) -> bool{ return (true); }));
}

bool Iface::closeSockets(SocketFilter filter) {
    SocketCollection left_sockets; 
    left_sockets.reserve(sockets_.size());

    for(auto &sock_info : sockets_) {
        if (filter(sock_info)) {
            close(sock_info.sockfd_);
            if (sock_info.fallbackfd_ > 0) {
                close(sock_info.fallbackfd_);
            }
        } else {
            left_sockets.push_back(sock_info);
        }
    }

    if (sockets_.size() != left_sockets.size()) {
        sockets_ = std::move(left_sockets);
        return true;
    } else {
        return false;
    }
}

std::string Iface::getFullName() const {
    ostringstream tmp;
    tmp << name_ << "/" << ifindex_;
    return (tmp.str());
}

std::string Iface::getPlainMac() const {
    ostringstream tmp;
    tmp.fill('0');
    tmp << hex;
    for (int i = 0; i < mac_len_; i++) {
        tmp.width(2);
        tmp <<  static_cast<int>(mac_[i]);
        if (i < mac_len_-1) {
            tmp << ":";
        }
    }
    return (tmp.str());
}

void Iface::setMac(const uint8_t* mac, size_t len) {
    if (len > MAX_MAC_LEN) {
        kea_throw(OutOfRange, "Interface " << getFullName()
                << " was detected to have link address of length "
                << len << ", but maximum supported length is "
                << MAX_MAC_LEN);
    }
    mac_len_ = len;
    memcpy(mac_, mac, len);
}

bool Iface::delAddress(const IOAddress& addr) {
    auto i = std::find(addrs_.begin(), addrs_.end(), addr);
    if (i != addrs_.end()) {
        addrs_.erase(i);
        return (true);
    } else {
        return (false);
    }
}

bool Iface::delSocket(uint16_t sockfd) {
    return (closeSockets([sockfd](const SocketInfo& sock_info) -> bool {
            return (sock_info.sockfd_ == sockfd);
    }));
}

bool Iface::getAddress(IOAddress& address) const {
    if (addrs_.size() > 0) {
        address = addrs_[0];
        return (true);
    }  else {
        return (false);
    }
}

bool Iface::hasAddress(const IOAddress& addr) const {
    return (std::find(addrs_.begin(), addrs_.end(), addr) != addrs_.end());
}

void Iface::addAddress(const IOAddress& addr) {
    if (hasAddress(addr)) {
        kea_throw(BadValue, "Address " << addr
                << " already defined on the " << name_ << " interface.");
    }
    addrs_.push_back(addr);
}

void Iface::addUnicast(const IOAddress& addr) {
    if (std::find(unicasts_.begin(), unicasts_.end(), addr) != unicasts_.end()){
        kea_throw(BadValue, "Address " << addr
                << " already defined on the " << name_ << " interface.");
    }
    unicasts_.push_back(addr);
}

bool Iface::isLoopBack() const {
    return (flags_ & IFF_LOOPBACK);
}

bool Iface::isUp() const {
    return (flags_ & IFF_UP);
}

bool Iface::isRunning() const {
    return (flags_ & IFF_RUNNING);
}

bool Iface::isMulticast() const {
    return (flags_ & IFF_MULTICAST);
}

bool Iface::isBroadcast() const {
    return (flags_ & IFF_BROADCAST);
}

};
};
