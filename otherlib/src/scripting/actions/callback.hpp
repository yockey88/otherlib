/**
 * \file scripting/actions/callback.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_ACTIONS_CALLBACK_HPP
#define OTHERLIB_SCRIPTING_ACTIONS_CALLBACK_HPP

#include <tuple>
#include <type_traits>

#include "core/value.hpp"

#include "dotnet/dotnet_object.hpp"
#include "lua/lua_script.hpp"

namespace other {

  class dotnet_object;

  struct callback_error : public std::runtime_error {
    explicit callback_error(const std::string& message)
        : std::runtime_error(message) {}
  };

  template <typename R>
  concept acceptable_return = std::is_same_v<R, void> || std::is_pointer_v<R> || std::is_trivial_v<R>;

  template <typename R>
  constexpr static inline bool is_acceptable_callback_return = acceptable_return<typename std::invoke_result_t<R>>;

  struct callback {
    virtual ~callback() = default;
    value call(const std::span<value> args);

   protected:
    virtual value call_impl(const std::span<value> args) = 0;
  };

  template <typename R, typename... Args>
    requires acceptable_return<R>
  struct native_callback : public callback {
    using callback_type = R (*)(Args...);
    using argument_tuple_type = std::tuple<Args...>;

    /// normal function types
    native_callback(callback_type func)
        : function(func) {}
    /// ctor for lambas and functors
    template <typename F>
      requires std::is_convertible_v<F, callback_type>
    native_callback(F&& func)
        : function(static_cast<callback_type>(func)) {}

    virtual ~native_callback() = default;

    value call_impl(const std::span<value> args) override {
      if (args.size() != sizeof...(Args)) {
        throw callback_error("Invalid number of arguments for native callback.");
      }

      argument_tuple_type unpacked_args = detail::unpack_args<Args...>(args);
      if constexpr (std::is_same_v<R, void>) {
        std::apply(function, unpacked_args);
        return value();
      } else {
        return value(std::apply(function, unpacked_args));
      }
    }

   private:
    callback_type function;
  };

  template <typename R, typename... Args>
    requires acceptable_return<R>
  struct dotnet_callback : public callback {
    using argument_tuple_type = std::tuple<Args...>;

    dotnet_callback(dotnet_object* obj, const std::string_view method_name)
        : dotnet_obj(obj), method_name(method_name) {}
    virtual ~dotnet_callback() = default;

    value call_impl(const std::span<value> args) override {
      if (dotnet_obj == nullptr) {
        throw callback_error("Dotnet object is null in dotnet callback.");
      }

      auto unpacked_args = detail::unpack_args<Args...>(args);
      if constexpr (std::is_same_v<R, void>) {
        std::apply([this](auto&&... unpacked) { dotnet_obj->invoke<void>(method_name, std::forward<Args>(unpacked)...); }, unpacked_args);
        return value();
      } else {
        R ret = std::apply([this](auto&&... unpacked) { return dotnet_obj->invoke<R>(method_name, std::forward<Args>(unpacked)...); }, unpacked_args);
        return value(ret);
      }
    }

   private:
    dotnet_object* dotnet_obj = nullptr;
    std::string method_name = "";
  };

  template <typename R, typename... Args>
  struct lua_callback : public callback {
    using argument_tuple_type = std::tuple<Args...>;

    lua_callback(lua_script* script, const std::string_view function_name)
        : lua_script_ptr(script), function_name(function_name) {}
    virtual ~lua_callback() = default;
    value call_impl(const std::span<value> args) override {
      if (lua_script_ptr == nullptr || !lua_script_ptr->is_valid()) {
        throw callback_error("Lua script is not valid in lua callback.");
      }

      auto unpacked_args = detail::unpack_args<Args...>(args);
      if constexpr (std::is_same_v<R, void>) {
        std::apply([this](auto&&... unpacked) { lua_script_ptr->call_function<void>(function_name, std::forward<Args>(unpacked)...); }, unpacked_args);
        return value();
      } else {
        R ret = std::apply([this](auto&&... unpacked) { return lua_script_ptr->call_function<R>(function_name, std::forward<Args>(unpacked)...); }, unpacked_args);
        return value(ret);
      }
    }

   private:
    lua_script* lua_script_ptr = nullptr;
    std::string function_name = "";
  };

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_ACTIONS_CALLBACK_HPP