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
  // constexpr static inline auto fmtstr(const std::string_view format, Args&&... args) {
  //   return std::format(std::format("{}", format), std::forward<Args>(args)...);
  // }

  // template <>
  // constexpr inline auto fmtstr(const std::string_view line) {
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

namespace std {

  template <>
  struct formatter<glm::vec2> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const glm::vec2& vec, FormatContext& ctx) const {
      return formatter<std::string_view>::format(std::format("vec2({}, {})", vec.x, vec.y), ctx);
    }
  };

  template <>
  struct formatter<glm::vec3> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const glm::vec3& vec, FormatContext& ctx) const {
      return formatter<std::string_view>::format(std::format("vec3({}, {}, {})", vec.x, vec.y, vec.z), ctx);
    }
  };

  template <>
  struct formatter<glm::vec4> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const glm::vec4& vec, FormatContext& ctx) const {
      return formatter<std::string_view>::format(std::format("vec4({}, {}, {}, {})", vec.x, vec.y, vec.z, vec.w), ctx);
    }
  };

  template <>
  struct formatter<glm::ivec2> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const glm::ivec2& vec, FormatContext& ctx) const {
      return formatter<std::string_view>::format(std::format("ivec2({}, {})", vec.x, vec.y), ctx);
    }
  };

  template <>
  struct formatter<glm::ivec3> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const glm::ivec3& vec, FormatContext& ctx) const {
      return formatter<std::string_view>::format(std::format("ivec3({}, {}, {})", vec.x, vec.y, vec.z), ctx);
    }
  };

  template <>
  struct formatter<glm::ivec4> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const glm::ivec4& vec, FormatContext& ctx) const {
      return formatter<std::string_view>::format(std::format("ivec4({}, {}, {}, {})", vec.x, vec.y, vec.z, vec.w), ctx);
    }
  };

  template <>
  struct formatter<glm::mat2> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const glm::mat2& mat, FormatContext& ctx) const {
      return formatter<std::string_view>::format(std::format("mat2(\n  {}, {},\n  {}, {}\n)", mat[0][0], mat[0][1], mat[1][0], mat[1][1]), ctx);
    }
  };

  template <>
  struct formatter<glm::mat3> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const glm::mat3& mat, FormatContext& ctx) const {
      return formatter<std::string_view>::format(std::format("mat3(\n  {}, {}, {},\n  {}, {}, {},\n  {}, {}, {}\n)", mat[0][0], mat[0][1], mat[0][2], mat[1][0], mat[1][1], mat[1][2], mat[2][0], mat[2][1], mat[2][2]), ctx);
    }
  };

  template <>
  struct formatter<glm::mat4> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const glm::mat4& mat, FormatContext& ctx) const {
      return formatter<std::string_view>::format(std::format("mat4(\n  {}, {}, {}, {},\n  {}, {}, {}, {},\n  {}, {}, {}, {},\n  {}, {}, {}, {}\n)", mat[0][0], mat[0][1], mat[0][2], mat[0][3], mat[1][0], mat[1][1], mat[1][2], mat[1][3], mat[2][0], mat[2][1], mat[2][2], mat[2][3], mat[3][0], mat[3][1], mat[3][2], mat[3][3]), ctx);
    }
  };

}  // namespace std

#endif  // OTHER_CORE_FORMATTING_HPP