#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <list>
#include <memory>

namespace kea {
namespace util {

class ThreadPool {
public:
  static const size_t kDefaultMaxCachedThreads = 1;

  ThreadPool(size_t max_cached_threads = kDefaultMaxCachedThreads);

  void run(std::function<void()> task);

protected:
  void startThread();

  size_t max_cached_threads_;
  size_t num_threads_;
  std::atomic<int> free_threads_;
  std::mutex runq_mutex_;
  std::list<std::function<void()>> runq_;
  std::condition_variable wakeup_;
};

};
};
