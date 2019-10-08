#include <stdarg.h>
#include <syslog.h>

namespace kea {
namespace log {
void open_log() {
    openlog("kea_slave", LOG_NDELAY|LOG_PID, LOG_LOCAL7);
}

void close_log() {
    closelog();
}

void log_debug(const char* fmt, ...) {
    va_list ap;
    va_start(ap,fmt);
    vsyslog(LOG_DEBUG, fmt, ap);
    va_end(ap);
}

void log_info(const char* fmt, ...) {
    va_list ap;
    va_start(ap,fmt);
    vsyslog(LOG_INFO, fmt, ap);
    va_end(ap);
}

void log_notice(const char* fmt, ...) {
    va_list ap;
    va_start(ap,fmt);
    vsyslog(LOG_NOTICE, fmt, ap);
    va_end(ap);
}

void log_warnning(const char* fmt, ...) {
    va_list ap;
    va_start(ap,fmt);
    vsyslog(LOG_WARNING, fmt, ap);
    va_end(ap);
}

void log_error(const char* fmt, ...) {
    va_list ap;
    va_start(ap,fmt);
    vsyslog(LOG_ERR, fmt, ap);
    va_end(ap);
}

void log_fatal(const char* fmt, ...) {
    va_list ap;
    va_start(ap,fmt);
    vsyslog(LOG_EMERG, fmt, ap);
    va_end(ap);
}
};
};
