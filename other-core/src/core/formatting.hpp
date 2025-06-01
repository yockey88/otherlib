/**
 * \file core/formatting.hpp
 **/
#ifndef OTHER_CORE_FORMATTING_HPP
#define OTHER_CORE_FORMATTING_HPP

#include <format>
#include <iostream>
#include <print>
#include <string_view>

#include <glm/glm.hpp>

#include "core/defines.hpp"

namespace other {

  using namespace std::literals::string_view_literals;

  // template <typename... Args>
  // static inline void println(const std::string_view format, Args&&... args) {
  //   std::println(std::cout, format, std::forward<Args>(args)...);
  // }

  // template <>
  // inline void println(const std::string_view line) {
  //   println(line);
  // }

  // template <typename... Args>
  // static inline auto fmtstr(const std::string_view format, Args&&... args) {
  //   return std::format(std::format("{}", format), std::forward<Args>(args)...);
  // }

  // template <>
  // inline auto fmtstr(const std::string_view line) {
  //   return std::string{ line };
  // }

  // template <typename... Args>
  // static inline auto fmterr(const std::string_view format, Args&&... args) {
  //   /// TODO: something else...
  //   return fmtstr(format, std::forward<Args>(args)...);
  // }

  // template <>
  // inline auto fmterr(const std::string_view line) {
  //   return fmtstr(line);
  // }

  // template <typename T>
  // static inline auto fmtopt(const std::string_view format, const opt<T>& opt) {
  //   if (opt.has_value()) {
  //     return fmtstr(format, opt.value());
  //   } else {
  //     return fmtstr("ERR");
  //   }
  // }

}  // namespace other

std::ostream& operator<<(std::ostream& os, const glm::vec2& vec);
std::ostream& operator<<(std::ostream& os, const glm::vec3& vec);
std::ostream& operator<<(std::ostream& os, const glm::vec4& vec);

std::ostream& operator<<(std::ostream& os, const glm::ivec2& vec);
std::ostream& operator<<(std::ostream& os, const glm::ivec3& vec);
std::ostream& operator<<(std::ostream& os, const glm::ivec4& vec);

std::ostream& operator<<(std::ostream& os, const glm::mat2& mat);
std::ostream& operator<<(std::ostream& os, const glm::mat3& mat);
std::ostream& operator<<(std::ostream& os, const glm::mat4& mat);

#endif  // OTHER_CORE_FORMATTING_HPP