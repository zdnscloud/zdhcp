#include <string>
#include <assert.h>
#include <kea/logging/stringutil.h>

namespace kea{
namespace logging{

template <>
std::string StringUtil::toString(std::string value) {
    return value;
}

template <>
std::string StringUtil::toString(const char* value) {
    return value;
}

template <>
std::string StringUtil::toString(char* value) {
    return value;
}

template <>
std::string StringUtil::toString(int value) {
    return std::to_string(value);
}

template <>
std::string StringUtil::toString(unsigned value) {
    return std::to_string(value);
}

template <>
std::string StringUtil::toString(unsigned short value) {
    return std::to_string(value);
}

template <>
std::string StringUtil::toString(long value) {
    return std::to_string(value);
}

template <>
std::string StringUtil::toString(unsigned long value) {
    return std::to_string(value);
}

template <>
std::string StringUtil::toString(long long value) {
    return std::to_string(value);
}

template <>
std::string StringUtil::toString(unsigned long long value) {
    return std::to_string(value);
}

template <>
std::string StringUtil::toString(unsigned char value) {
    return std::to_string(value);
}

template <>
std::string StringUtil::toString(void* value) {
    return "<ptr>";
}

template <>
std::string StringUtil::toString(const void* value) {
    return "<ptr>";
}

template <>
std::string StringUtil::toString(double value) {
    char buf[128]; 
    *buf = 0;

    auto len = snprintf(buf, sizeof(buf), "%f", value);
    if (len < 0) {
        RAISE(kRuntimeError, "snprintf() failed");
    }

    while (len > 2 && buf[len - 1] == '0' && buf[len - 2] != '.') {
        buf[len--] = 0;
    }

    return std::string(buf, len);
}

template <>
std::string StringUtil::toString(bool value) {
    return value ? "true" : "false";
}

void StringUtil::replaceAll(
    std::string* str,
    const std::string& pattern,
    const std::string& replacement) {
  if (str->size() == 0) {
    return;
  }

  auto cur = 0;
  while((cur = str->find(pattern, cur)) != std::string::npos) {
    str->replace(cur, pattern.size(), replacement);
    cur += replacement.size();
  }
}

}
}
