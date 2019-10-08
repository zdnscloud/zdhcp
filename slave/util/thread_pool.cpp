#include <kea/util/thread_pool.h>

#include <cassert>
#include <memory>
#include <thread>
#include <unistd.h>
#include <iostream>

namespace kea {
namespace util {

ThreadPool::ThreadPool(size_t max_cached_threads) :
    max_cached_threads_(max_cached_threads),
    num_threads_(0),
    free_threads_(0) {}

void ThreadPool::run(std::function<void()> task) {
    std::unique_lock<std::mutex> l(runq_mutex_);
    runq_.push_back(task);

    if (runq_.size() > free_threads_) {
        startThread();
    }

    l.unlock();
    wakeup_.notify_one();
}


void ThreadPool::startThread() {
    bool cache = false;
    num_threads_ += 1;
    if (num_threads_ <= max_cached_threads_) {
        cache = true;
    }

    std::thread thread([this, cache] () {
        do {
            try {
                std::unique_lock<std::mutex> lk(runq_mutex_);
                free_threads_++;

                while (runq_.size() == 0) {
                    wakeup_.wait(lk); 
                }

                assert(runq_.size() > 0);
                auto task = runq_.front();
                runq_.pop_front();
                free_threads_--;
                lk.unlock();

                task();
            } catch (const std::exception& e) {
            }
        } while (cache);


        std::unique_lock<std::mutex> lk(runq_mutex_);
        --num_threads_;
    });

    thread.detach();
}


};
};
