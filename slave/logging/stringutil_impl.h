#pragma once

#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <iostream>
#include <kea/logging/stringutil.h>

namespace kea{
namespace logging{

template <typename ValueType, typename... T>
void StringUtil::formatImpl(
    std::string* scratch,
    int argn,
    ValueType value,
    T... values) {
  StringUtil::replaceAll(
      scratch,
      "$" + std::to_string(argn),
      StringUtil::toString(value));

  formatImpl(scratch, argn + 1, values...);
}

template <typename ValueType>
void StringUtil::formatImpl(
    std::string* scratch,
    int argn,
    ValueType value) {
  StringUtil::replaceAll(
      scratch,
      "$" + std::to_string(argn),
      StringUtil::toString(value));
}

template <typename... T>
std::string StringUtil::format(const char* fmt, T... values) {
  std::string str = fmt;
  StringUtil::formatImpl(&str, 0, values...);
  return str;
}

template <typename... T>
std::string StringUtil::format(const std::string& fmt, T... values) {
  std::string str = fmt;
  StringUtil::formatImpl(&str, 0, values...);
  return str;
}

}
}
