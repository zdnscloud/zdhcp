#include <kea/ping/timer_queue.h>

namespace kea{
namespace pinger{

static const int32_t NEGNIGIBLE_DELAY = 10; //milliseconds

TimerQueue::TimerQueue(uint32_t max_size, MilliSeconds timeout) : max_size_(max_size), time_out_(timeout) {
    stop_.store(false);
    std::thread t_pop([&](){removeExpireTimers();});
    thread_remove_timer_ = std::move(t_pop);
}

TimerQueue::~TimerQueue() {
    stop();
}

void TimerQueue::stop() {
    if (stop_.load()) {
        return;
    }
    stop_.store(true);
    timer_queue_.push({(std::chrono::system_clock::now() - time_out_), [&](){}});
    empty_condition_.notify_one();
    thread_remove_timer_.join();
    std::queue<TimerNode> empty_queue;
    timer_queue_.swap(empty_queue);
}

bool TimerQueue::addTimer(TimerCallBack callback) {
    std::unique_lock<std::mutex> lock(timer_mutex_);
    if (timer_queue_.size() >= max_size_) {
        return false;
    }
    timer_queue_.push({(std::chrono::system_clock::now() + time_out_), callback});
    lock.unlock();
    empty_condition_.notify_one();
    return true;
}

void 
TimerQueue::removeExpireTimers() {
    std::vector<TimerCallBack> callbacks;
    int32_t sleep_time;
    while(true) {
        if (stop_.load()) { break; }
        sleep_time = 0;
        std::unique_lock<std::mutex> lock(timer_mutex_);
        empty_condition_.wait(lock, [this]{return !(timer_queue_.empty());});
        auto now_time = std::chrono::system_clock::now();
        while (!timer_queue_.empty()) {
            auto expire_time = timer_queue_.front().time_out_;
            int32_t time_diff = std::chrono::duration_cast<MilliSeconds>(expire_time - now_time).count();
            if (time_diff > NEGNIGIBLE_DELAY) { 
                sleep_time = time_diff;
                break;
            } else {
                TimerNode tn = timer_queue_.front();
                timer_queue_.pop();
                callbacks.push_back(tn.call_back_);
            }
        }
        lock.unlock();

        for (auto callback : callbacks) {
            callback();
        }

        if (sleep_time > 0) {
            std::this_thread::sleep_for(MilliSeconds(sleep_time));
        }

        callbacks.clear();
    }
}

};
};
