#include <cerrno>
#include <cstring>

#include <kea/ping/ping.h>
#include <kea/logging/logging.h>


using namespace kea::logging;

namespace kea{
namespace pinger{

static Pinger* SingletonPinger = nullptr;

const uint32_t PACKET_SIZE = 128;
const uint32_t PROTO_ICMP = 1;

Pinger::Pinger(uint32_t max_size, bool is_enable, MilliSeconds time_out) : is_enable_(is_enable), timer_queue_(max_size, time_out){
    pipe(pipefd_);
    stop_.store(false);
    fcntl(pipefd_[1], F_SETFL, O_NONBLOCK);
    if ((sockfd_ = socket(AF_INET, SOCK_RAW, PROTO_ICMP)) <= 0) {
        logError("Ping      ", "!!!create socket error");
        assert(sockfd_ > 0);
    }
    fcntl(sockfd_, F_SETFL, O_NONBLOCK);
    std::thread threadrecv([&](){Pinger::recvPacket();});  
    thread_recv_ = std::move(threadrecv);
}

Pinger::~Pinger() {
    stop();
}

void Pinger::stop() {
    if(stop_.load()) { return; }
    stop_.store(true);
    timer_queue_.stop();
    write(pipefd_[1], "1", 1);
    thread_recv_.join();
    seq_record_map_.clear();
    close(sockfd_);
    close(pipefd_[0]);
    close(pipefd_[1]);
}

void Pinger::init(bool is_enable, uint32_t time_out) {
    if(SingletonPinger != nullptr) {
        SingletonPinger->stop();
        delete SingletonPinger;
    }
        
    MilliSeconds timeout(time_out*1000);
    SingletonPinger = new Pinger(QUEUE_MAX_SIZE, is_enable, timeout);
    std::atexit([](){ delete SingletonPinger; });
}

Pinger& 
Pinger::instance() {
    return *SingletonPinger;
}

void Pinger::ping(ClientContextPtr client_ctx, ClientContextHandler callback) {
    if(!is_enable_) {
        callback(std::move(client_ctx));
        return;
    }

    uint16_t pack_seq = pack_seq_.fetch_add(1);
    uint16_t random = random_.getRandom();
    uint32_t pack_id = (random << 16) + pack_seq;
    IOAddress allocated_addr = client_ctx->getYourAddr();
    if(addTimerTarget(pack_id)) {
        addPingTarget(pack_id, std::move(client_ctx), callback);
        sendPacket(allocated_addr, pack_seq, random);  
    }
}

bool Pinger::notifyPingTarget(uint32_t pack_id, bool reachable) {
    std::unique_lock<std::mutex> lock(mutex_);
    auto it = seq_record_map_.find(pack_id) ;
    if (it != seq_record_map_.end()) {   
        PingRecord record = it->second;
        seq_record_map_.erase(it);
        lock.unlock();
        record.client_ctx_->setRequestAddrConflict(reachable);
        record.val_(std::move(record.client_ctx_));
        return true;
    } else {
        lock.unlock();
        return false;
    }
}

bool Pinger::addTimerTarget(uint32_t pack_id) {
    return timer_queue_.addTimer(std::bind(&Pinger::notifyPingTarget, this, pack_id, false));
}

void Pinger::addPingTarget(uint32_t pack_id, ClientContextPtr client_ctx, ClientContextHandler callback) {
    std::lock_guard<std::mutex> guard(mutex_);
    seq_record_map_.insert(std::pair<uint32_t, PingRecord>(pack_id, {std::move(client_ctx), callback}));
}

void Pinger::sendPacket(IOAddress ip_addr, uint16_t pack_seq, uint16_t random) {   
    char send_packet[PACKET_SIZE];
    struct sockaddr_in dest_addr;

    bool is_ip = getsockaddr(ip_addr.toText().c_str(), &dest_addr);
    if (!is_ip) {
        logError("Ping      ", "IP given $0 is not valid", ip_addr.toText());
        assert(is_ip == true);
    }

    uint32_t packet_size = packIcmp(pack_seq, random, (struct icmp*)send_packet); 

    if ((sendto(sockfd_, send_packet, packet_size, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr))) < 0 ) {
        logError("Ping      ", "Send ping packet failed with ip $0 with error: $1", ip_addr.toText(), std::strerror(errno));
    }
}

void Pinger::recvPacket() {       
    uint32_t packet_len;
    uint32_t pack_id = 0;
    fd_set rset;     
    char recv_packet[PACKET_SIZE];
    uint32_t max_fds = sockfd_ > pipefd_[0] ? sockfd_ : pipefd_[0];
    while (true) {
        FD_ZERO(&rset);
        FD_SET(sockfd_, &rset);
        FD_SET(pipefd_[0], &rset);

        if ((select(max_fds + 1, &rset, NULL, NULL, NULL)) == -1 ) {
            logError("Ping      ", "!!! select() failed");
        }

        if (FD_ISSET(pipefd_[0], &rset)) {
            char readbuf[2];
            read(pipefd_[0], readbuf, 1);
            break;
        }

        if (FD_ISSET(sockfd_, &rset)) {
            if ((packet_len = recvfrom(sockfd_, recv_packet, sizeof(recv_packet), 0, NULL, NULL)) < 0) {
                continue;
            }
        }

        if (!unpackIcmp(recv_packet, packet_len, pack_id)) {
            continue;
        }

        notifyPingTarget(pack_id, true);
    }
}

};
};
