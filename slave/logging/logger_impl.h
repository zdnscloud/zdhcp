#pragma once

#include "kea/logging/stringutil.h"

namespace kea{
namespace logging{

template <typename... T>
void Logger::log(
    LogLevel log_level,
    const std::string& component,
    const std::string& message,
    T... args) {
  if (log_level >= min_level_) {
    log(log_level, component, StringUtil::format(message, args...));
  }
}

template <typename... T>
void Logger::logException(
    LogLevel log_level,
    const std::string& component,
    const std::exception& exception,
    const std::string& message,
    T... args) {
  if (log_level >= min_level_) {
    logException(
        log_level,
        component,
        exception,
        StringUtil::format(message, args...));
  }
}

}
}
