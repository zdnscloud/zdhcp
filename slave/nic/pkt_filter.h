#pragma once
#include <kea/dhcp++/pkt.h>
#include <kea/exceptions/exceptions.h>

using namespace kea::dhcp;

namespace kea {
namespace nic {

class InvalidPacketFilter : public Exception {
public:
    InvalidPacketFilter(const char* file, size_t line, const char* what) :
        Exception(file, line, what) { };
};

class SocketConfigError : public Exception {
public:
    SocketConfigError(const char* file, size_t line, const char* what) :
        Exception(file, line, what) { }; 
};

struct SocketInfo;
class Iface;

class PktFilter {
public:
    virtual ~PktFilter() { }

    virtual bool isDirectResponseSupported() const = 0;
    virtual SocketInfo openSocket(Iface& iface, const IOAddress& addr,
            const uint16_t port, const bool receive_bcast, const bool send_bcast) = 0;

    virtual std::unique_ptr<Pkt> receive(Iface&, const SocketInfo&) = 0;
    virtual int send(Iface&, uint16_t, Pkt&) = 0;

protected:
    virtual int openFallbackSocket(const IOAddress& addr, const uint16_t port);
};

};
};
