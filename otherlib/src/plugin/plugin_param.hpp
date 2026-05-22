/**
 * \file plugin/plugin_param.hpp
 **/
#ifndef OTHERLIB_PLUGIN_PLUGIN_PARAM_HPP
#define OTHERLIB_PLUGIN_PLUGIN_PARAM_HPP

namespace other {

  struct plugin_param {
    const char* name;
    const char* value;
  };

  struct plugin_param_view {
    const plugin_param* params = nullptr;
    size_t count = 0;

    bool empty() const { return count == 0 || params == nullptr; }
    bool contains(const std::string_view param_name) const {
      if (!empty()) {
        for (size_t i = 0; i < count; ++i) {
          if (params[i].name == param_name) {
            return true;
          }
        }
      }
      return false;
    }
    opt<std::string_view> get(const std::string_view param_name) const {
      if (!empty()) {
        for (size_t i = 0; i < count; ++i) {
          if (params[i].name == param_name) {
            return params[i].value;
          }
        }
      }
      return std::nullopt;
    }
    std::string_view get_or(const std::string_view param_name, const std::string_view default_value) const {
      auto value = get(param_name);
      return value.has_value() ? *value : default_value;
    }

    constexpr const plugin_param* begin() const { return params; }
    constexpr const plugin_param* end() const { return params + count; }
  };

  template <size_t N>
  struct plugin_param_storage {
    static constexpr size_t size = N;
    plugin_param entries[N];

    constexpr plugin_param_view view() const noexcept {
      return plugin_param_view{ entries, N };
    }
  };

  template <>
  struct plugin_param_storage<0> {
    static constexpr size_t size = 0;

    constexpr plugin_param_view view() const noexcept {
      return plugin_param_view{};
    }
  };

  template <typename... Ts>
    requires(std::same_as<std::remove_cvref_t<Ts>, plugin_param> && ...)
  constexpr auto make_param_storage(Ts&&... ts) {
    return plugin_param_storage<sizeof...(Ts)>{ { std::forward<Ts>(ts)... } };
  }

  template <>
  constexpr auto make_param_storage<>() {
    return plugin_param_storage<0>{};
  }

}  // namespace other

#define OTHER_PARAM(key_str, value_str) \
  ::other::plugin_param { (key_str), (value_str) }

#define OTHER_PARAMS(...)                                                 \
  ([]() constexpr -> ::other::plugin_param_view {                         \
    static constexpr ::other::plugin_param __entries[] = { __VA_ARGS__ }; \
    return ::other::plugin_param_view{ __entries, std::size(__entries) }; \
  }())

// #define OTHER_PARAMS(target, ...)                           \
//   ([]() constexpr -> ::other::plugin_param_view {           \
//     return ::other::make_param_storage(__VA_ARGS__).view(); \
//   }())

#endif  // OTHERLIB_PLUGIN_PLUGIN_PARAM_HPP