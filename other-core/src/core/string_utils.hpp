/**
 * \file core/string_utils.hpp
 **/
#ifndef OTHER_CORE_CORE_STRING_UTILS_HPP
#define OTHER_CORE_CORE_STRING_UTILS_HPP

#include <string>

namespace other {

  static inline void trim_front(std::string& str) {
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) { return !std::isspace(ch); }));
  }
  static inline void trim_back(std::string& str) {
    str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), str.end());
  }

  static inline void trim_front_and_back(std::string& str) {
    trim_front(str);
    trim_back(str);
  }

}  // namespace other

#endif  // OTHER_CORE_CORE_STRING_UTILS_HPP