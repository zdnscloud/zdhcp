#pragma once

#include "kea/logging/loglevel.h"
#include "kea/logging/logger.h"


namespace kea{
namespace logging{

template <typename... T>
void logEmergency(const std::string& component, const std::string& msg, T... args) {
  Logger::get()->log(LogLevel::kEmergency, component, msg, args...);
}

template <typename... T>
void logEmergency(
    const std::string& component,
    const std::exception& e,
    const std::string& msg,
    T... args) {
  Logger::get()->logException(LogLevel::kEmergency, component, e, msg, args...);
}

template <typename... T>
void logAlert(const std::string& component, const std::string& msg, T... args) {
  Logger::get()->log(LogLevel::kAlert, component, msg, args...);
}

template <typename... T>
void logAlert(
    const std::string& component,
    const std::exception& e,
    const std::string& msg,
    T... args) {
  Logger::get()->logException(LogLevel::kAlert, component, e, msg, args...);
}

template <typename... T>
void logCritical(const std::string& component, const std::string& msg, T... args) {
  Logger::get()->log(LogLevel::kCritical, component, msg, args...);
}

template <typename... T>
void logCritical(
    const std::string& component,
    const std::exception& e,
    const std::string& msg,
    T... args) {
  Logger::get()->logException(LogLevel::kCritical, component, e, msg, args...);
}

template <typename... T>
void logError(const std::string& component, const std::string& msg, T... args) {
  Logger::get()->log(LogLevel::kError, component, msg, args...);
}

template <typename... T>
void logError(
    const std::string& component,
    const std::exception& e,
    const std::string& msg,
    T... args) {
  Logger::get()->logException(LogLevel::kError, component, e, msg, args...);
}

template <typename... T>
void logWarning(const std::string& component, const std::string& msg, T... args) {
  Logger::get()->log(LogLevel::kWarning, component, msg, args...);
}

template <typename... T>
void logWarning(
    const std::string& component,
    const std::exception& e,
    const std::string& msg,
    T... args) {
  Logger::get()->logException(LogLevel::kWarning, component, e, msg, args...);
}

template <typename... T>
void logNotice(const std::string& component, const std::string& msg, T... args) {
  Logger::get()->log(LogLevel::kNotice, component, msg, args...);
}

template <typename... T>
void logNotice(
    const std::string& component,
    const std::exception& e,
    const std::string& msg,
    T... args) {
  Logger::get()->logException(LogLevel::kNotice, component, e, msg, args...);
}

template <typename... T>
void logInfo(const std::string& component, const std::string& msg, T... args) {
  Logger::get()->log(LogLevel::kInfo, component, msg, args...);
}

template <typename... T>
void logInfo(
    const std::string& component,
    const std::exception& e,
    const std::string& msg,
    T... args) {
  Logger::get()->logException(LogLevel::kInfo, component, e, msg, args...);
}

template <typename... T>
void logDebug(const std::string& component, const std::string& msg, T... args) {
  Logger::get()->log(LogLevel::kDebug, component, msg, args...);
}

template <typename... T>
void logDebug(
    const std::string& component,
    const std::exception& e,
    const std::string& msg,
    T... args) {
  Logger::get()->logException(LogLevel::kDebug, component, e, msg, args...);
}

template <typename... T>
void logTrace(const std::string& component, const std::string& msg, T... args) {
  Logger::get()->log(LogLevel::kTrace, component, msg, args...);
}

template <typename... T>
void logTrace(
    const std::string& component,
    const std::exception& e,
    const std::string& msg,
    T... args) {
  Logger::get()->logException(LogLevel::kTrace, component, e, msg, args...);
}

const char* logLevelToStr(LogLevel log_level);

LogLevel strToLogLevel(const std::string& log_level);

}
}
