/**
 * \file scripting/actions/callback.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_ACTIONS_CALLBACK_HPP
#define OTHERLIB_SCRIPTING_ACTIONS_CALLBACK_HPP

#include <tuple>
#include <type_traits>
#include <vector>

#include "core/value.hpp"

#include "dotnet/dotnet_object.hpp"
#include "lua/lua_script.hpp"

#include "forward.hpp"
#include "table.hpp"


namespace other {

  class dotnet_object;

  struct callback_error : public std::runtime_error {
    explicit callback_error(const std::string& message)
        : std::runtime_error(message) {}
  };

  enum class callback_type {
    NATIVE,
    DOTNET,
    LUA,
  };

  struct callback : public ref_counted {
    virtual ~callback() = default;

    template <typename R = void, typename... Args>
    R call(Args&&... args);

   protected:
    callback(callback_type type) : type(type) {}

   private:
    callback_type type;
  };

  template <typename R, typename... Args>
  struct native_callback : public callback {
    using callback_fn_type = std::function<R(Args...)>;
    using argument_tuple_type = std::tuple<Args...>;

    template <typename F>
      requires std::is_invocable_r_v<R, F, Args...>
    native_callback(F&& func) : callback(callback_type::NATIVE), function(func) {}
    native_callback(callback_fn_type func) : callback(callback_type::NATIVE), function(func) {}
    virtual ~native_callback() = default;

    R call_direct(Args&&... args) {
      if (!function) {
        throw callback_error("Native callback function is not set.");
      }

      return function(std::forward<Args>(args)...);
    }

   private:
    callback_fn_type function;
  };
  template <typename R, typename... Args>
  native_callback(R (*)(Args...)) -> native_callback<R, Args...>;
  template <typename R, typename... Args>
  native_callback(std::function<R(Args...)>) -> native_callback<R, Args...>;

  struct dotnet_callback : public callback {
    dotnet_callback(dotnet_object* obj, const std::string_view method_name)
        : callback(callback_type::DOTNET), dotnet_obj(obj), method_name(method_name) {}
    virtual ~dotnet_callback() = default;

    template <typename Ret, typename... CallArgs>
    Ret call_direct(CallArgs&&... args) {
      if (!dotnet_obj) {
        throw callback_error("Dotnet object is null in dotnet callback.");
      }

      return invoke_with_conversion<Ret, CallArgs...>(std::forward<CallArgs>(args)...);
    }

   private:
    dotnet_object* dotnet_obj = nullptr;
    std::string method_name = "";

    template <typename Arg>
    static auto convert_arg(Arg&& arg) {
      if constexpr (std::is_same_v<std::decay_t<Arg>, std::string>) {
        return native_string(arg.c_str());
      } else if constexpr (std::is_same_v<std::decay_t<Arg>, std::string_view>) {
        return native_string(std::string(arg).c_str());
      } else {
        return std::forward<Arg>(arg);
      }
    }

    template <typename Ret, typename... CallArgs>
    Ret invoke_with_conversion(CallArgs&&... args) {
      auto converted_args = std::make_tuple(convert_arg(std::forward<CallArgs>(args))...);
      return std::apply(
        [this](auto&&... converted) { return dotnet_obj->invoke<Ret>(method_name, std::forward<decltype(converted)>(converted)...); },
        converted_args
      );
    }
  };

  struct lua_callback : public callback {
    lua_callback(sol::function func)
        : callback(callback_type::LUA), lua_function(func) {}
    lua_callback(sol::function func, const std::string_view function_name)
        : callback(callback_type::LUA), lua_function(func), function_name(function_name) {}
    lua_callback(sol::table table, const std::string_view function_name)
        : callback(callback_type::LUA), lua_table(table), function_name(function_name) {}
    lua_callback(lua_script* script, const std::string_view function_name)
        : callback(callback_type::LUA), script(script), function_name(function_name) {}
    virtual ~lua_callback() = default;

    template <typename Ret, typename... CallArgs>
    Ret call_direct(CallArgs&&... args) {
      if (lua_function.has_value()) {
        return call_function<Ret>(*lua_function, std::forward<CallArgs>(args)...);
      }

      if (script != nullptr) {
        const bool has_func = script->has_symbol(function_name);
        if (!has_func) {
          throw callback_error(std::format("Lua script does not have function '{}' for lua callback.", function_name));
        }

        return script->call_function<Ret, CallArgs...>(function_name, std::forward<CallArgs>(args)...);
      }

      if (lua_table.valid()) {
        sol::object func_obj = lua_table[function_name.data()];
        if (!func_obj.valid() || func_obj.get_type() != sol::type::function) {
          throw callback_error(std::format("Lua function '{}' not found in lua callback table.", function_name));
        }
        sol::function func = func_obj.as<sol::function>();
        return call_function<Ret>(func, std::forward<CallArgs>(args)...);
      }

      throw callback_error("No valid Lua function found in lua callback.");
    }

   private:
    lua_script* script = nullptr;
    opt<sol::function> lua_function;
    sol::table lua_table;
    std::string function_name = "";

    template <typename Ret, typename... CallArgs>
    Ret call_function(sol::function func, CallArgs&&... args) {
      try {
        sol::object result = func(std::forward<CallArgs>(args)...);
        if constexpr (std::is_same_v<Ret, void>) {
          return;
        } else {
          if (result.is<Ret>()) {
            return result.as<Ret>();
          } else {
            throw callback_error(std::format("Lua function '{}' did not return expected type.", function_name));
          }
        }
      } catch (const sol::error& e) {
        throw callback_error("Error invoking Lua function: " + std::string(e.what()));
      } catch (...) {
        throw callback_error("Unknown error invoking Lua function.");
      }
    }
  };

  template <typename R, typename... Args>
  R callback::call(Args&&... args) {
    switch (type) {
      case callback_type::NATIVE: return reinterpret_cast<native_callback<R, Args...>*>(this)->call_direct(std::forward<Args>(args)...);
      case callback_type::DOTNET: return reinterpret_cast<dotnet_callback*>(this)->template call_direct<R, Args...>(std::forward<Args>(args)...);
      case callback_type::LUA: return reinterpret_cast<lua_callback*>(this)->template call_direct<R, Args...>(std::forward<Args>(args)...);
      default:
        throw callback_error("Unknown callback type.");
    }
  }

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_ACTIONS_CALLBACK_HPP