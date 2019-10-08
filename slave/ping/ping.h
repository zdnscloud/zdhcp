#pragma once 

#include <iostream>
#include <sys/time.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include <chrono>
#include <fcntl.h>
#include <map>
#include <functional>
#include <mutex>
#include <thread>
#include <atomic>
#include <assert.h>
#include <kea/ping/timer_queue.h>
#include <kea/ping/random_generate.h>
#include <kea/ping/icmp_protocol.h>
#include <kea/util/io_address.h>
#include <kea/client/client_context_wrapper.h>

using namespace kea::util;

namespace kea{
namespace pinger{

typedef std::chrono::milliseconds MilliSeconds;

using PingRecord = client::ClientContextWrapper<client::ClientContextHandler>;
using kea::client::ClientContextPtr;
using kea::client::ClientContextHandler;

class Pinger {

    public:
        static const uint32_t QUEUE_MAX_SIZE = 4096;

        explicit Pinger(uint32_t max_size, bool is_enable, MilliSeconds time_out = MilliSeconds(1000));
        ~Pinger();

        Pinger(const Pinger& ) = delete;
        Pinger& operator=(const Pinger&) = delete;

        void ping(ClientContextPtr client_ctx, ClientContextHandler callback);
        static void init(bool is_enable, uint32_t time_out);
        static Pinger& instance();
        void stop();

    private:
        void sendPacket(IOAddress ip_addr, uint16_t pack_seq, uint16_t random);
        void recvPacket();
        bool notifyPingTarget(uint32_t pack_id, bool reachable);
        void addPingTarget(uint32_t pack_id, ClientContextPtr client_ctx, ClientContextHandler callback);
        bool addTimerTarget(uint32_t pack_id);

        bool is_enable_;
        uint32_t sockfd_;
        std::map<uint32_t, PingRecord> seq_record_map_;
        std::mutex mutex_;
        std::thread thread_recv_;
        RandomGenerate<uint16_t> random_;
        TimerQueue timer_queue_;
        int pipefd_[2];
        std::atomic<uint16_t> pack_seq_;
        std::atomic<bool> stop_;
};

};
};
