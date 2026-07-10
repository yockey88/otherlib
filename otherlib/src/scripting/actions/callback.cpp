/**
 * \file scripting/actions/callback.cpp
 **/
#include "scripting/actions/callback.hpp"

namespace other {
  namespace detail {

    // inline sol::object value_to_sol_object(lua_State* L, const value& v) {
    //   switch (v.type()) {
    //     case value_type::OEBOOL: return sol::make_object(L, static_cast<bool>(v));
    //     case value_type::CHAR: return sol::make_object(L, static_cast<char>(v));
    //     case value_type::INT8: return sol::make_object(L, static_cast<int8_t>(v));
    //     case value_type::INT16: return sol::make_object(L, static_cast<int16_t>(v));
    //     case value_type::INT32: return sol::make_object(L, static_cast<int32_t>(v));
    //     case value_type::INT64: return sol::make_object(L, static_cast<integer_t>(v));
    //     case value_type::UINT8: return sol::make_object(L, static_cast<uint8_t>(v));
    //     case value_type::UINT16: return sol::make_object(L, static_cast<uint16_t>(v));
    //     case value_type::UINT32: return sol::make_object(L, static_cast<uint32_t>(v));
    //     case value_type::UINT64: return sol::make_object(L, static_cast<natural_t>(v));
    //     case value_type::FLOAT: return sol::make_object(L, static_cast<float>(v));
    //     case value_type::DOUBLE: return sol::make_object(L, static_cast<double>(v));
    //     case value_type::STRING: return sol::make_object(L, v.as_string());
    //     case value_type::BYTE_BUFFER: {
    //       std::span<const uint8_t> data_span = v.as_byte_buffer();
    //       return sol::make_object(L, ostd::vector<uint8_t>(data_span.begin(), data_span.end()));
    //     }
    //     default: return sol::make_object(L, sol::lua_nil);
    //   }
    // }

  }  // namespace detail
}  // namespace other