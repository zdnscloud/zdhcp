#pragma once

#include <kea/logging/fileoutputstream.h>
#include <kea/logging/loglevel.h>
#include <kea/logging/logtarget.h>

namespace kea{
namespace logging{

class LogFileOutputStream : public LogTarget {
public:
  LogFileOutputStream(
      const std::string& program_name,
      std::unique_ptr<FileOutputStream> target);

  void log(
      LogLevel level,
      const std::string& component,
      const std::string& message) override;

private:
  const std::string getCurrentSysTime();

  std::string program_name_;
  std::unique_ptr<FileOutputStream> target_;

};

}
}
