#include <kea/ping/icmp_protocol.h>

namespace kea{
namespace pinger{

const uint32_t ICMP_DATA_LEN = 56;
const uint32_t ICMP_HEAD_LEN = 8;

unsigned short getChksum(unsigned short *addr, uint32_t len) {   
    uint32_t nleft = len;
    uint32_t sum = 0;
    unsigned short *w = addr;
    unsigned short answer = 0;

    while (nleft > 1) {   
        sum += *w++;
        nleft -= 2;
    }

    if ( nleft == 1) {   
        *(unsigned char *)(&answer)=*(unsigned char *)w;
        sum += answer;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;

    return answer;
}

uint32_t packIcmp(uint16_t pack_seq, uint16_t random, struct icmp* icmp) {   
    struct icmp *tmp_icmp = icmp;
    tmp_icmp->icmp_type=ICMP_ECHO;
    tmp_icmp->icmp_code=0;
    tmp_icmp->icmp_cksum=0;
    tmp_icmp->icmp_seq=pack_seq;
    tmp_icmp->icmp_id= random;
    uint32_t pack_size = ICMP_HEAD_LEN + ICMP_DATA_LEN;
    tmp_icmp->icmp_cksum=getChksum((unsigned short *)icmp,pack_size); 

    return pack_size;
}

bool unpackIcmp(char *packet_buf, uint32_t packet_len, uint32_t &pack_id) {   
    struct ip *ip = (struct ip *)packet_buf;
    uint32_t iphdr_len = ip->ip_hl << 2;    
    struct icmp* icmp = (struct icmp *)(packet_buf + iphdr_len);  
    packet_len -= iphdr_len;            

    if (packet_len < ICMP_HEAD_LEN) {   
        return false;
    }

    if (icmp->icmp_type != ICMP_ECHOREPLY) {
        return false;
    }

    pack_id = (icmp->icmp_id << 16) + icmp->icmp_seq;
    return true;
}

bool getsockaddr(const char * ip_addr, struct sockaddr_in* sock_addr) {
    struct sockaddr_in dest_addr;
    bzero(&dest_addr, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;

    if (inet_addr(ip_addr) == INADDR_NONE) {    
        return false;
    } else if (!inet_aton(ip_addr, &dest_addr.sin_addr)) {  
        return false;
    }

    *sock_addr = dest_addr;
    return true;
}

};
};
