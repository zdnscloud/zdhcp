#include <kea/ping/ping.h>
#include <gflags/gflags.h>

using namespace kea::pinger;

DECLARE_string(testip);
DEFINE_string(testip, "10.0.2.15", "test ip for ping");

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Usage: ping_test --testip= \n";
        return 0;
    }
    ::google::ParseCommandLineFlags(&argc, &argv, true);
    int sockfd = socket(AF_INET, SOCK_RAW, 1);
    char send_packet[128];
    struct sockaddr_in dest_addr;

    if (!getsockaddr(FLAGS_testip.c_str(), &dest_addr)) {
        std::cout<<"testip is not a ip\n";
        return 0;
    }

    std::random_device random;
    std::uniform_int_distribution<> distribution;
    uint16_t random_num = distribution(random);
    uint16_t pack_seq = 111;
    uint32_t packet_size = packIcmp(pack_seq, random_num, (struct icmp*)send_packet); 

    if ((sendto(sockfd, send_packet, packet_size, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr))) < 0 ) {
        std::cout<< "send error\n";
        return 0;
    }  

    uint32_t pack_seq_id = (random_num << 16) + pack_seq;
    uint32_t packet_len;
    uint32_t pack_id = 0;
    uint32_t nfd = 0;
    bool is_ping = false;
    fd_set rset;
    char recv_packet[128];
    uint32_t max_fds = sockfd;
    struct timeval timeout;
    FD_ZERO(&rset);

    for (int i=0; i<4; ++i) {
        FD_SET(sockfd, &rset);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        if ((nfd = select(max_fds + 1, &rset, NULL, NULL, &timeout)) == -1 ) {
            continue;
        }

        if (nfd == 0) {
            continue;
        }

        if (FD_ISSET(sockfd, &rset)) {
            if ((packet_len = recvfrom(sockfd, recv_packet, sizeof(recv_packet), 0, NULL, NULL)) < 0) {
                continue;
            }
        }

        if (!unpackIcmp(recv_packet, packet_len, pack_id)) {
            continue;
        }

        if (pack_id == pack_seq_id) {
            is_ping = true;
            std::cout << "testip has been used\n";
            break;
        }
    }

    if (!is_ping) {
        std::cout << "testip is unreachable\n";
    }

    return 0;
}
