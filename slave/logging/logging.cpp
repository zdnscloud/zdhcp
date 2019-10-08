#include <sstream>
#include <stdarg.h>
#include <kea/logging/logging.h>
#include <kea/logging/exception.h>
#include <kea/logging/logfileoutputstream.h>
#include <kea/logging/syslogtarget.h>

namespace kea{
namespace logging{

const char* logLevelToStr(LogLevel log_level) {
  switch (log_level) {
    case LogLevel::kError: return "EROR";
    case LogLevel::kWarning: return "WARN";
    case LogLevel::kInfo: return "INFO";
    case LogLevel::kDebug: return "DEBG";
    default: return "CUST"; 
  }
}

LogLevel strToLogLevel(const std::string& log_level) {
  if (log_level == "error") return LogLevel::kError;
  if (log_level == "info") return LogLevel::kInfo;
  if (log_level == "warn") return LogLevel::kWarning;
  if (log_level == "debug") return LogLevel::kDebug;
  RAISEF(kIllegalArgumentError, "unknown log level: $0", log_level);
}

Logger* Logger::get() {
  static Logger singleton;
  return &singleton;
}

Logger::Logger() :
    min_level_(LogLevel::kNotice),
    max_listener_index_(0) {
  for (int i = 0; i < STX_LOGGER_MAX_LISTENERS; ++i) {
    listeners_[i] = nullptr;
  }
}

void Logger::logException(
        LogLevel log_level,
        const std::string& component,
        const std::exception& exception,
        const std::string& message) {
    if (log_level < min_level_) {
        return;
    }

    try {
        auto rte = dynamic_cast<const Exception&>(exception);
        log(
                log_level,
                component,
                "$0: $1: $2\n    in $3\n    in $4:$5",
                message,
                rte.getTypeName(),
                rte.getMessage(),
                rte.method(),
                rte.file(),
                rte.line());
    } catch (const std::exception& bcee) {
        log(
                log_level,
                component,
                "$0: std::exception: <foreign exception> $1",
                message,
                exception.what());
    }
}

void Logger::log(
      LogLevel log_level,
      const std::string& component,
      const std::string& message) {
  if (log_level < min_level_) {
    return;
  }

  const auto max_idx = max_listener_index_.load();
  for (int i = 0; i < max_idx; ++i) {
    auto listener = listeners_[i].load();

    if (listener != nullptr) {
      listener->log(log_level, component, message);
    }
  }
}

void Logger::addTarget(LogTarget* target) {
  auto listener_id = max_listener_index_.fetch_add(1);
  listeners_[listener_id] = target;
}

void Logger::setMinimumLogLevel(LogLevel min_level) {
  min_level_ = min_level;
}

void Logger::open_log(const std::string& log_file_path, const std::string& program_name, LogLevel min_log_level, bool log_enable){
    Logger::get()->setMinimumLogLevel(min_log_level);

    if (log_enable && (!log_file_path.empty())) {
        Logger::get()->addTarget(new LogFileOutputStream(program_name, FileOutputStream::openFile(log_file_path)));
    }

    Logger::get()->addTarget(new SyslogTarget(program_name));
}

}
}
