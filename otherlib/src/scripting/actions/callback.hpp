/**
 * \file scripting/actions/callback.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_ACTIONS_CALLBACK_HPP
#define OTHERLIB_SCRIPTING_ACTIONS_CALLBACK_HPP

#include <tuple>
#include <type_traits>
#include <vector>

#include <sol/sol.hpp>

#include "core/value.hpp"

#include "dotnet/dotnet_object.hpp"
#include "lua/lua_script.hpp"

namespace other {

  class dotnet_object;

  struct callback_error : public std::runtime_error {
    explicit callback_error(const std::string& message)
        : std::runtime_error(message) {}
  };

  struct callback : public ref_counted {
    virtual ~callback() = default;
    value call(const std::span<value> args);

   protected:
    virtual value call_impl(const std::span<value> args) = 0;
  };

  template <typename R, typename... Args>
  struct native_callback : public callback {
    using callback_type = std::function<R(Args...)>;
    using argument_tuple_type = std::tuple<Args...>;

    /// normal function types
    native_callback(callback_type func)
        : function(func) {}
    /// ctor for lambas and functors
    template <typename F>
      requires std::is_invocable_r_v<R, F, Args...>
    native_callback(F&& func)
        : function(func) {}

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
  native_callback(R (*)(Args...)) -> native_callback<R, Args...>;

  template <typename R, typename... Args>
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
    lua_callback(sol::function func)
        : lua_func(func) {}
    lua_callback(sol::function func, const std::string_view function_name)
        : lua_func(func), function_name(function_name) {}
    virtual ~lua_callback() = default;
    value call_impl(const std::span<value> args) override {
      if (lua_script_ptr == nullptr) {
        if (!lua_func.valid()) {
          CORE_LOG_ERROR("Lua function is not valid in lua callback.");
          return value();
        }

        auto unpacked_args = detail::unpack_args<Args...>(args);
        if constexpr (std::is_same_v<R, void>) {
          std::apply([this](auto&&... unpacked) { lua_func(std::forward<Args>(unpacked)...); }, unpacked_args);
          return value();
        } else {
          R ret = std::apply([this](auto&&... unpacked) { return lua_func(std::forward<Args>(unpacked)...); }, unpacked_args);
          return value(ret);
        }
      } else {
        if (!lua_script_ptr->is_valid()) {
          CORE_LOG_ERROR("Lua script is not valid in lua callback.");
          return value();
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
    }

   private:
    lua_script* lua_script_ptr = nullptr;
    sol::function lua_func;
    std::string function_name = "";
  };

  struct lua_table_callback : public callback {
    lua_table_callback(sol::table table, const std::string_view method)
        : interface_table(std::move(table)), method_name(method) {}
    virtual ~lua_table_callback() = default;

    value call_impl(const std::span<value> args) override;

   private:
    sol::table interface_table;
    std::string method_name;
  };

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_ACTIONS_CALLBACK_HPP