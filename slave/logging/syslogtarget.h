#pragma once

#include <kea/logging/logtarget.h>
#include <memory>

namespace kea{
namespace logging{

class SyslogTarget : public LogTarget {
public:

  explicit SyslogTarget(const std::string& name);
  ~SyslogTarget();

  void log(
      LogLevel level,
      const std::string& component,
      const std::string& message) override;
};

}
}
