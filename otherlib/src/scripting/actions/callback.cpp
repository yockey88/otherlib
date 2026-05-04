/**
 * \file scripting/actions/callback.cpp
 **/
#include "scripting/actions/callback.hpp"

namespace other {
  namespace detail {

    inline sol::object value_to_sol_object(lua_State* L, const value& v) {
      switch (v.type()) {
        case value_type::OEBOOL: return sol::make_object(L, static_cast<bool>(v));
        case value_type::CHAR: return sol::make_object(L, static_cast<char>(v));
        case value_type::INT8: return sol::make_object(L, static_cast<int8_t>(v));
        case value_type::INT16: return sol::make_object(L, static_cast<int16_t>(v));
        case value_type::INT32: return sol::make_object(L, static_cast<int32_t>(v));
        case value_type::INT64: return sol::make_object(L, static_cast<integer_t>(v));
        case value_type::UINT8: return sol::make_object(L, static_cast<uint8_t>(v));
        case value_type::UINT16: return sol::make_object(L, static_cast<uint16_t>(v));
        case value_type::UINT32: return sol::make_object(L, static_cast<uint32_t>(v));
        case value_type::UINT64: return sol::make_object(L, static_cast<natural_t>(v));
        case value_type::FLOAT: return sol::make_object(L, static_cast<float>(v));
        case value_type::DOUBLE: return sol::make_object(L, static_cast<double>(v));
        case value_type::STRING: return sol::make_object(L, v.as_string());
        case value_type::BYTE_BUFFER: {
          std::span<const uint8_t> data_span = v.as_byte_buffer();
          return sol::make_object(L, std::vector<uint8_t>(data_span.begin(), data_span.end()));
        }
        default: return sol::make_object(L, sol::lua_nil);
      }
    }

  }  // namespace detail

  value callback::call(const std::span<value> args) {
    try {
      return call_impl(args);
    } catch (const callback_error& e) {
      CORE_LOG_ERROR("Callback error: {}", e.what());
      return value();
    } catch (const std::exception& e) {
      CORE_LOG_ERROR("Unexpected error during callback execution: {}", e.what());
      return value();
    } catch (...) {
      CORE_LOG_ERROR("Unknown error during callback execution.");
      return value();
    }
  }

  value lua_table_callback::call_impl(const std::span<value> args) {
    sol::object func_obj = interface_table[method_name];
    if (!func_obj.valid() || func_obj.get_type() != sol::type::function) {
      CORE_LOG_ERROR("Lua table callback could not find method '{}'.", method_name);
      return value{};
    }

    sol::function func = func_obj.as<sol::function>();
    lua_State* L = func.lua_state();

    std::vector<sol::object> sol_args;
    sol_args.reserve(args.size());
    for (const value& v : args) {
      sol_args.push_back(detail::value_to_sol_object(L, v));
    }

    try {
      func(sol::as_args(sol_args));
    } catch (const sol::error& e) {
      CORE_LOG_ERROR("Error in lua table callback '{}': {}", method_name, e.what());
    }

    return value{};
  }

}  // namespace other