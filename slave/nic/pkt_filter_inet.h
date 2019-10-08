#pragma once

#include <kea/nic/pkt_filter.h>
#include <kea/util/io_address.h>

#include <cstdint>

namespace kea {
namespace nic {

class PktFilterInet : public PktFilter {
public:
    PktFilterInet();
    ~PktFilterInet();

    virtual bool isDirectResponseSupported() const {
        return (false);
    }

    virtual SocketInfo openSocket(Iface& iface,
                                  const kea::util::IOAddress& addr,
                                  uint16_t port,
                                  const bool receive_bcast,
                                  const bool send_bcast);

    virtual std::unique_ptr<Pkt> receive(Iface& iface, const SocketInfo& socket_info);

    virtual int send(Iface& iface, uint16_t sockfd, Pkt& pkt);

private:
    uint8_t *control_buf_;
    size_t control_buf_len_;
};

}; 
};
