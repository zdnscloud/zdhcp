#pragma once


namespace kea {
namespace log {

void open_log();

void close_log();

void log_debug(const char* fmt, ...);

void log_info(const char* fmt, ...);

void log_notice(const char* fmt, ...);

void log_warnning(const char* fmt, ...);

void log_error(const char* fmt, ...);

void log_fatal(const char* fmt, ...);

};
};
