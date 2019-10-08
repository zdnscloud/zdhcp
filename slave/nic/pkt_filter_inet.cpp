#include <kea/dhcp++/pkt.h>
#include <kea/nic/pkt_filter_inet.h>
#include <kea/nic/iface.h>
#include <kea/nic/iface_mgr.h>

#include <errno.h>
#include <cstring>
#include <fcntl.h>

using namespace kea::dhcp;

namespace kea {
namespace nic {

PktFilterInet::PktFilterInet() {
    control_buf_len_ = CMSG_SPACE(sizeof(struct in6_pktinfo));
    control_buf_ = new uint8_t[control_buf_len_];
}

PktFilterInet::~PktFilterInet() {
    delete []control_buf_;
}

SocketInfo PktFilterInet::openSocket(Iface& iface,
        const IOAddress& addr, uint16_t port,
        bool receive_bcast, bool send_bcast) {
    struct sockaddr_in addr4;
    memset(&addr4, 0, sizeof(sockaddr));
    addr4.sin_family = AF_INET;
    addr4.sin_port = htons(port);

    // If we are to receive broadcast messages we have to bind
    // to "ANY" address.
    if (receive_bcast && iface.isBroadcast()) {
        addr4.sin_addr.s_addr = INADDR_ANY;
    } else {
        addr4.sin_addr.s_addr = htonl(addr);
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        kea_throw(SocketConfigError, "Failed to create UDP6 socket.");
    }

    if (fcntl(sock, F_SETFD, FD_CLOEXEC) < 0) {
        close(sock);
        kea_throw(SocketConfigError, "Failed to set close-on-exec flag"
                  << " on socket " << sock);
    }

#ifdef SO_BINDTODEVICE
    if (receive_bcast && iface.isBroadcast()) {
        // Bind to device so as we receive traffic on a specific interface.
        if (setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, iface.getName().c_str(),
                       iface.getName().length() + 1) < 0) {
            close(sock);
            kea_throw(SocketConfigError, "Failed to set SO_BINDTODEVICE option"
                      << " on socket " << sock);
        }
    }
#endif

    if (send_bcast && iface.isBroadcast()) {
        int flag = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &flag, sizeof(flag)) < 0) {
            close(sock);
            kea_throw(SocketConfigError, "Failed to set SO_BROADCAST option"
                      << " on socket " << sock);
        }
    }

    if (bind(sock, (struct sockaddr *)&addr4, sizeof(addr4)) < 0) {
        close(sock);
        kea_throw(SocketConfigError, "Failed to bind socket " << sock
                  << " to " << addr
                  << "/port=" << port);
    }

#if defined(IP_PKTINFO)
    int flag = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_PKTINFO, &flag, sizeof(flag)) != 0) {
        close(sock);
        kea_throw(SocketConfigError, "setsockopt: IP_PKTINFO: failed.");
    }
#endif

    SocketInfo sock_desc(addr, port, sock);
    return (sock_desc);

}

std::unique_ptr<Pkt> PktFilterInet::receive(Iface& iface, 
        const SocketInfo& socket_info) {
    struct sockaddr_in from_addr;
    uint8_t buf[IfaceMgr::RCVBUFSIZE];
    memset(control_buf_, 0, control_buf_len_);
    memset(&from_addr, 0, sizeof(from_addr));
    struct msghdr m;
    memset(&m, 0, sizeof(m));
    m.msg_name = &from_addr;
    m.msg_namelen = sizeof(from_addr);
    struct iovec v;
    v.iov_base = static_cast<void*>(buf);
    v.iov_len = IfaceMgr::RCVBUFSIZE;
    m.msg_iov = &v;
    m.msg_iovlen = 1;
    m.msg_control = &control_buf_[0];
    m.msg_controllen = control_buf_len_;

    int result = recvmsg(socket_info.sockfd_, &m, 0);
    if (result < 0) {
        kea_throw(SocketReadError, "failed to receive UDP4 data");
    }

    std::unique_ptr<Pkt> pkt;
    try {
        Pkt* p = new Pkt(buf, result);
        pkt.reset(p);
        pkt->updateTimestamp();
        pkt->setIfaceIndex(iface.getIndex());
        pkt->setIface(iface.getName());
        pkt->setRemoteAddr(IOAddress(htonl(from_addr.sin_addr.s_addr)));
        pkt->setRemotePort(htons(from_addr.sin_port));
        pkt->setLocalPort(socket_info.port_);
    } catch (const kea::Exception& ) { 
        return nullptr;
    }  

    struct cmsghdr* cmsg;
    struct in_pktinfo* pktinfo;
    //struct in_addr to_addr;
    //memset(&to_addr, 0, sizeof(to_addr));
    cmsg = CMSG_FIRSTHDR(&m);
    while (cmsg != NULL) {
        if ((cmsg->cmsg_level == IPPROTO_IP) &&
            (cmsg->cmsg_type == IP_PKTINFO)) {
            pktinfo = (struct in_pktinfo*)CMSG_DATA(cmsg);
            pkt->setIfaceIndex(pktinfo->ipi_ifindex);
            pkt->setLocalAddr(IOAddress(htonl(pktinfo->ipi_addr.s_addr)));
            // to_addr = pktinfo->ipi_spec_dst;
            break;
        }
        cmsg = CMSG_NXTHDR(&m, cmsg);
    }

    return (std::move(pkt));
}

int PktFilterInet::send(Iface&, uint16_t sockfd, Pkt& pkt) {
    memset(control_buf_, 0, control_buf_len_);

    // Set the target address we're sending to.
    sockaddr_in to;
    memset(&to, 0, sizeof(to));
    to.sin_family = AF_INET;
    to.sin_port = htons(pkt.getRemotePort());
    to.sin_addr.s_addr = htonl(pkt.getRemoteAddr());

    struct msghdr m;
    // Initialize our message header structure.
    memset(&m, 0, sizeof(m));
    m.msg_name = &to;
    m.msg_namelen = sizeof(to);

    // Set the data buffer we're sending. (Using this wacky
    // "scatter-gather" stuff... we only have a single chunk
    // of data to send, so we declare a single vector entry.)
    struct iovec v;
    memset(&v, 0, sizeof(v));
    // iov_base field is of void * type. We use it for packet
    // transmission, so this buffer will not be modified.
    v.iov_base = const_cast<void *>(pkt.getBuffer().getData());
    v.iov_len = pkt.getBuffer().getLength();
    m.msg_iov = &v;
    m.msg_iovlen = 1;

// In the future the OS-specific code may be abstracted to a different
// file but for now we keep it here because there is no code yet, which
// is specific to non-Linux systems.
#if defined (IP_PKTINFO) && defined (OS_LINUX)
    // Setting the interface is a bit more involved.
    //
    // We have to create a "control message", and set that to
    // define the IPv4 packet information. We set the source address
    // to handle correctly interfaces with multiple addresses.
    m.msg_control = &control_buf_[0];
    m.msg_controllen = control_buf_len_;
    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&m);
    cmsg->cmsg_level = IPPROTO_IP;
    cmsg->cmsg_type = IP_PKTINFO;
    cmsg->cmsg_len = CMSG_LEN(sizeof(struct in_pktinfo));
    struct in_pktinfo* pktinfo =(struct in_pktinfo *)CMSG_DATA(cmsg);
    memset(pktinfo, 0, sizeof(struct in_pktinfo));
    pktinfo->ipi_ifindex = pkt.getIndex();
    pktinfo->ipi_spec_dst.s_addr = htonl(pkt.getLocalAddr()); // set the source IP address
    m.msg_controllen = CMSG_SPACE(sizeof(struct in_pktinfo));
#endif

    pkt.updateTimestamp();

    int result = sendmsg(sockfd, &m, 0);
    if (result < 0) {
        kea_throw(SocketWriteError, "pkt4 send failed: sendmsg() returned "
                  "from " << pkt.getLocalAddr().toText() << "to " << pkt.getRemoteAddr().toText() <<
                  " with an error: " << strerror(errno));
    }

    return (result);
}

};
};
