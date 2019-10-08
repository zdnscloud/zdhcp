#include <thread>
#include <condition_variable>
#include <queue>
#include <chrono>
#include <iostream>
#include <cstdlib>
#include <functional>
#include <mutex>
#include <atomic>

namespace kea{
namespace pinger{

typedef std::function<void()> TimerCallBack;
typedef std::chrono::milliseconds MilliSeconds;

struct TimerNode{
    std::chrono::system_clock::time_point time_out_;
    TimerCallBack call_back_;
};

class TimerQueue {
    public:
        TimerQueue(uint32_t max_size, MilliSeconds time_out);
        ~TimerQueue();

        TimerQueue(const TimerQueue&) = delete;
        TimerQueue& operator=(const TimerQueue&) = delete;

        bool addTimer(TimerCallBack callback);
        void stop();
        void setTimeOut(uint32_t time_out);

    private:
        void removeExpireTimers();

        uint32_t max_size_;
        std::chrono::milliseconds time_out_;
        std::mutex timer_mutex_;
        std::condition_variable empty_condition_;
        std::queue<TimerNode> timer_queue_;
        std::atomic<bool> stop_;
        std::thread thread_remove_timer_;
};

};
};
