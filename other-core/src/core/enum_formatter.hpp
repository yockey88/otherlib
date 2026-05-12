/**
 * \file core/enum_formatter.hpp
 **/
#ifndef OTHER_CORE_CORE_ENUM_FORMATTER_HPP
#define OTHER_CORE_CORE_ENUM_FORMATTER_HPP

#include <string_view>
#include <type_traits>

#include <magic_enum/magic_enum.hpp>

namespace std {

  template <typename T>
    requires std::is_enum_v<T>
  struct formatter<T> : public formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const T& value, FormatContext& ctx) const {
      auto enum_name = magic_enum::enum_name(value);
      if (enum_name.empty()) {
        return formatter<std::string_view>::format("Invalid enum value", ctx);
      }
      return formatter<std::string_view>::format(enum_name, ctx);
    }
  };

}  // namespace std

#endif  // OTHER_CORE_CORE_ENUM_FORMATTER_HPP