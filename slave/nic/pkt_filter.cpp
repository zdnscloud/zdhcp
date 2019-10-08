#include <kea/nic/pkt_filter.h>

#include <sys/fcntl.h>
#include <sys/socket.h>

namespace kea {
namespace nic {

int PktFilter::openFallbackSocket(const IOAddress& addr, uint16_t port) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        kea_throw(SocketConfigError, "failed to create fallback socket for"
                " address " << addr << ", port " << port
                << ", reason: " << strerror(errno));
    }

    if (fcntl(sock, F_SETFD, FD_CLOEXEC) < 0) {
        close(sock);
        kea_throw(SocketConfigError, "Failed to set close-on-exec flag"
                << " on fallback socket for address " << addr
                << ", port " << port
                << ", reason: " << strerror(errno));
    }

    struct sockaddr_in addr4;
    memset(&addr4, 0, sizeof(addr4));
    addr4.sin_family = AF_INET;
    addr4.sin_addr.s_addr = htonl(addr);
    addr4.sin_port = htons(port);
    if (bind(sock, reinterpret_cast<struct sockaddr*>(&addr4),
                sizeof(addr4)) < 0) {
        char* errmsg = strerror(errno);
        close(sock);
        kea_throw(SocketConfigError, "failed to bind fallback socket to"
                " address " << addr << ", port " << port
                << ", reason: " << errmsg
                << " - is another DHCP server running?");
    }

    if (fcntl(sock, F_SETFL, O_NONBLOCK) != 0) {
        char* errmsg = strerror(errno);
        close(sock);
        kea_throw(SocketConfigError, "failed to set SO_NONBLOCK option on the"
                " fallback socket, bound to " << addr << ", port "
                << port << ", reason: " << errmsg);
    }
    return (sock);
}

}; 
};
