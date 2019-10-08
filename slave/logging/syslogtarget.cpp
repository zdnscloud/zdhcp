#include <kea/logging/syslogtarget.h>
#include <kea/logging/logging.h>
#include <kea/log/kea_log.h>
#include <cstring>
#include <syslog.h>
#include <stdarg.h>

using namespace kea::log;

namespace kea{
namespace logging{

SyslogTarget::SyslogTarget(const std::string& name)  {
  setlogmask(LOG_UPTO (LOG_DEBUG));
  auto ident = (char*) malloc(name.length() + 1);
  if (ident) {
    memcpy(ident, name.c_str(), name.length() + 1);
    openlog(ident, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL7);
  } else {
    openlog(NULL, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL7);
  }
}

SyslogTarget::~SyslogTarget()  {
  closelog();
}

void SyslogTarget::log(
    LogLevel level,
    const std::string& component,
    const std::string& message) {
    switch (level) {
        case LogLevel::kError: 
            log_error(message.c_str());
            break;
        case LogLevel::kWarning: 
            log_warnning(message.c_str());
            break;
        case LogLevel::kNotice: 
            log_notice(message.c_str());
            break;
        case LogLevel::kInfo: 
            log_info(message.c_str());
            break;
        case LogLevel::kDebug: 
        case LogLevel::kTrace: 
            log_debug(message.c_str());
            break;
    }
}

}
}
