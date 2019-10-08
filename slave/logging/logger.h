#pragma once

#include <atomic>
#include <kea/logging/loglevel.h>
#include <kea/logging/logtarget.h>

#define STX_LOGGER_MAX_LISTENERS 64

namespace kea{
namespace logging{

class Logger {
public:
  Logger();
  static Logger* get();

  void log(
      LogLevel log_level,
      const std::string& component,
      const std::string& message);

  template <typename... T>
  void log(
      LogLevel log_level,
      const std::string& component,
      const std::string& message,
      T... args);

  void logException(
      LogLevel log_level,
      const std::string& component,
      const std::exception& exception,
      const std::string& message);

  template <typename... T>
  void logException(
      LogLevel log_level,
      const std::string& component,
      const std::exception& exception,
      const std::string& message,
      T... args);

  void addTarget(LogTarget* target);
  void setMinimumLogLevel(LogLevel min_level);
  static void open_log(const std::string& log_file_path, const std::string& program_name, LogLevel min_log_level=LogLevel::kTrace, bool log_enable=false);

private:
  std::atomic<LogLevel> min_level_;
  std::atomic<size_t> max_listener_index_;
  std::atomic<LogTarget*> listeners_[STX_LOGGER_MAX_LISTENERS];
};

}
}

#include <kea/logging/logger_impl.h>
