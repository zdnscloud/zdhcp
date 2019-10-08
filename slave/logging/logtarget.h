#pragma once

#include <kea/logging/loglevel.h>
#include <string>

namespace kea{
namespace logging{

class LogTarget {
public:
  virtual ~LogTarget() {}

  virtual void log(
      LogLevel level,
      const std::string& component,
      const std::string& message) = 0;
};

}
}
