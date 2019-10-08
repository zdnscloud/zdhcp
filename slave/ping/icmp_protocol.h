#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <cstring>

namespace kea{
namespace pinger{
    uint32_t packIcmp(uint16_t pack_seq, uint16_t random, struct icmp* icmp);
    bool unpackIcmp(char *packet_buf, uint32_t packet_len, uint32_t & pack_id);
    unsigned short getChksum(unsigned short *addr,uint32_t len);
    bool getsockaddr(const char * ip_addr, sockaddr_in* sock_addr);
};
};
