#pragma once
#include <kea/dhcp++/pkt.h>
#include <kea/dhcp++/subnet.h>

namespace kea {
namespace server {
    using PktPtr = kea::dhcp::PktPtr;
    using Pkt = kea::dhcp::Pkt;
    using Subnet = kea::dhcp::Subnet;
    using IOAddress = kea::util::IOAddress;

    PktPtr genNakResponse(const Pkt& req);
    PktPtr genAckResponse(const Pkt& req, IOAddress ip_addr, const Subnet& subnet);
};
}
