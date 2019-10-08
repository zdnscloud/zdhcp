#include <chrono>
#include <kea/logging/logging.h>
#include <kea/logging/logfileoutputstream.h>
#include <kea/logging/stringutil.h>

namespace kea{
namespace logging{

LogFileOutputStream::LogFileOutputStream(
    const std::string& program_name,
    std::unique_ptr<FileOutputStream> target) :
    program_name_(program_name),
    target_(std::move(target)) {}

void LogFileOutputStream::log(
    LogLevel level,
    const std::string& component,
    const std::string& message) {
  const auto prefix = StringUtil::format(
      "$0 $1 [$2] ",
      getCurrentSysTime(),
      component.empty() ? program_name_ : component,
      logLevelToStr(level));

  std::string lines = prefix + message;
  StringUtil::replaceAll(&lines, "\n", "\n" + prefix);
  lines.append("\n");

  std::unique_lock<std::mutex> lk(target_->mutex);
  target_->write(lines);
}

const std::string LogFileOutputStream::getCurrentSysTime() {
    auto time_now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm* ptm = localtime(&time_now);
    char timedate[60] = {0};
    sprintf(timedate, "%d-%02d-%02d %02d:%02d:%02d",(int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday, 
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);

    return std::string(timedate);
}

}
}
