#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <locale>
#include <kea/logging/exception.h>

namespace kea{
namespace logging{

class StringUtil {
public:

  template <typename T>
  static std::string toString(T value);

  static void replaceAll(
      std::string* str,
      const std::string& pattern,
      const std::string& replacement = "");

  template <typename... T>
  static std::string format(const char* fmt, T... values);

  template <typename... T>
  static std::string format(const std::string& fmt, T... values);

private:
  template <typename ValueType, typename... T>
  static void formatImpl(
      std::string* scratch,
      int argn,
      ValueType value,
      T... values);

  template <typename ValueType>
  static void formatImpl(
      std::string* scratch,
      int argn,
      ValueType value);

};

}
}

#include <kea/logging/stringutil_impl.h>
