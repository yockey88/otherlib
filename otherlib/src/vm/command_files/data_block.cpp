/**
 * \file vm/command_files/data_block.cpp
 **/
#include "vm/command_files/data_block.hpp"

#include <cstdint>
#include <ranges>

#include "token.hpp"

namespace other {
  namespace {

    template <typename T>
      requires std::is_integral_v<T> || std::is_floating_point_v<T>
    ostd::vector<uint8_t> raw_data_from_value(const T& value) {
      ostd::vector<uint8_t> data(sizeof(T));
      std::memcpy(data.data(), &value, sizeof(T));
      return data;
    }

  }  // namespace

  data_type data_object::data_type_from_label(const std::string& label) {
    if (label == "byte") {
      return data_type::OCMD_DATA_TYPE_I8;
    } else if (label == "ubyte") {
      return data_type::OCMD_DATA_TYPE_U8;
    } else if (label == "int16") {
      return data_type::OCMD_DATA_TYPE_I16;
    } else if (label == "uint16") {
      return data_type::OCMD_DATA_TYPE_U16;
    } else if (label == "int32") {
      return data_type::OCMD_DATA_TYPE_I32;
    } else if (label == "uint32") {
      return data_type::OCMD_DATA_TYPE_U32;
    } else if (label == "float") {
      return data_type::OCMD_DATA_TYPE_F32;
    } else if (label == "double") {
      return data_type::OCMD_DATA_TYPE_F64;
    } else if (label == "string") {
      return data_type::OCMD_DATA_TYPE_STRING;
    } else if (label == "address") {
      return data_type::OCMD_DATA_TYPE_ADDRESS;
    } else if (label == "blob") {
      return data_type::OCMD_DATA_TYPE_BLOB;
    } else if (label == "user_type") {
      return data_type::OCMD_DATA_TYPE_USER_DEFINED;
    }

    return data_type::OCMD_DATA_TYPE_INVALID;
  }

  bool is_blob_string_without_0x_prefix(const std::string& str) {
    for (char c : str) {
      if (!std::isxdigit(static_cast<unsigned char>(c))) {
        return false;
      }
    }
    return true;
  }

  data_type data_object::deduce_data_type_from_tokens(const std::span<const token> value_tokens) {
    /// check if blob is of the form DE AD BE EF which shows as either all HEX_LITERAL or mix of HEX_LITERAL and INTEGER_LITERAL, but
    /// we don't want to misinterpret integer literals like "123456" as blob, we will interpret any thing more than two 123 456 as blob or 0x123456 as blob,

    const bool all_hex_literal = std::ranges::all_of(value_tokens, [](const token& tok) { return tok.type == TOKEN_TYPE_HEX_LITERAL; });
    const bool all_integer_literal = std::ranges::all_of(value_tokens, [](const token& tok) { return tok.type == TOKEN_TYPE_INTEGER_LITERAL; });
    const bool some_hex_literal = std::ranges::any_of(value_tokens, [](const token& tok) { return tok.type == TOKEN_TYPE_HEX_LITERAL; });
    const bool some_integer_literal = std::ranges::any_of(value_tokens, [](const token& tok) { return tok.type == TOKEN_TYPE_INTEGER_LITERAL; });
    const bool alL_string_literal = std::ranges::all_of(value_tokens, [](const token& tok) { return tok.type == TOKEN_TYPE_STRING_LITERAL; });
    const bool no_string_literal = std::ranges::none_of(value_tokens, [](const token& tok) { return tok.type == TOKEN_TYPE_STRING_LITERAL; });

    if (value_tokens.size() > 1) {
      if (all_hex_literal || all_integer_literal || (some_hex_literal && some_integer_literal && no_string_literal)) {
        return data_type::OCMD_DATA_TYPE_BLOB;
      } else if (alL_string_literal) {
        return data_type::OCMD_DATA_TYPE_STRING;
      } else {
        throw data_object_error("Cannot deduce data type for multiple tokens that are not all hex literals or all integer literals or all string literals");
      }
    } else {
      if (all_hex_literal) {
        return data_type::OCMD_DATA_TYPE_BLOB;
      } else if (all_integer_literal) {
        return data_type::OCMD_DATA_TYPE_I64;
      } else if (alL_string_literal) {
        return data_type::OCMD_DATA_TYPE_STRING;
      } else if (value_tokens[0].type == TOKEN_TYPE_FLOATING_POINT_LITERAL) {
        return data_type::OCMD_DATA_TYPE_F64;
      } else {
        throw data_object_error("Cannot deduce data type for single token that is not a hex literal or an integer literal or a string literal");
      }
    }
  }

  namespace {

    std::string trim_end(const std::string& str) {
      std::string result{ str };
      result.erase(std::find_if(result.rbegin(), result.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), result.end());
      return result;
    }

    std::string trim_beginning_and_end(const std::string& str) {
      std::string res = trim_end(str);
      res.erase(res.begin(), std::find_if(res.begin(), res.end(), [](unsigned char ch) { return !std::isspace(ch); }));
      return res;
    }

  }  // namespace

  ostd::vector<uint8_t> data_object::data_from_token_and_type(const token& value_token, data_type type) {
    switch (type) {
      case OCMD_DATA_TYPE_I8: {
        int16_t val = static_cast<int16_t>(std::stol(value_token.text));
        return raw_data_from_value<int16_t>(val);
      }
      case OCMD_DATA_TYPE_I16: {
        int64_t val = static_cast<int64_t>(std::stoll(value_token.text));
        return raw_data_from_value<int64_t>(val);
      }
      case OCMD_DATA_TYPE_I32: {
        int32_t val = static_cast<int32_t>(std::stol(value_token.text));
        return raw_data_from_value<int32_t>(val);
      }
      case OCMD_DATA_TYPE_I64: {
        int64_t val = static_cast<int64_t>(std::stoll(value_token.text));
        return raw_data_from_value<int64_t>(val);
      }
      case OCMD_DATA_TYPE_U8: {
        uint16_t val = static_cast<uint16_t>(std::stoul(value_token.text));
        return raw_data_from_value<uint16_t>(val);
      }
      case OCMD_DATA_TYPE_U16: {
        uint16_t val = static_cast<uint16_t>(std::stoul(value_token.text));
        return raw_data_from_value<uint16_t>(val);
      }
      case OCMD_DATA_TYPE_U32: {
        uint32_t val = static_cast<uint32_t>(std::stoul(value_token.text));
        return raw_data_from_value<uint32_t>(val);
      }
      case OCMD_DATA_TYPE_U64: {
        uint64_t val = static_cast<uint64_t>(std::stoull(value_token.text));
        return raw_data_from_value<uint64_t>(val);
      }
      case OCMD_DATA_TYPE_F32: return raw_data_from_value<float>(std::stof(value_token.text));
      case OCMD_DATA_TYPE_F64: return raw_data_from_value<double>(std::stod(value_token.text));
      case OCMD_DATA_TYPE_STRING:
        return value_token.text |
          std::views::transform([](char c) { return static_cast<uint8_t>(c); }) |
          std::ranges::to<ostd::vector<uint8_t>>();
      case OCMD_DATA_TYPE_ADDRESS: {
        const int base = value_token.text.starts_with("0x") ? 16 : 10;
        const uint16_t value = static_cast<uint16_t>(std::stoul(value_token.text, nullptr, base));
        return raw_data_from_value<uint16_t>(value);
      }

      case OCMD_DATA_TYPE_BLOB: {
        std::string raw_txt = value_token.text;
        if (raw_txt.starts_with("0x")) {
          raw_txt = value_token.text.substr(2);
        }

        return raw_txt |
          /// split data blob string on spaces and then trim white space and then filter if empty
          std::views::split(' ') |
          std::views::transform([](auto&& byte_str_view) { return trim_beginning_and_end(byte_str_view | std::ranges::to<std::string>()); }) |
          std::views::filter([](auto&& byte_str_view) { return !std::ranges::empty(byte_str_view); }) |
          /// turn each piece into a vector of uint8_t depending on how many bytes are in the string then flatten and collect
          std::views::transform([](auto&& byte_str_view) {
                 const std::string byte_str = byte_str_view | std::ranges::to<std::string>();
                 size_t num_bytes = byte_str.size() / 2 + (byte_str.size() % 2 != 0 ? 1 : 0);

                 ostd::vector<uint8_t> bytes = {};
                 for (size_t i = 0; i < num_bytes; ++i) {
                   bytes.push_back(static_cast<uint8_t>(std::stoul(byte_str.substr(i * 2, 2), nullptr, 16)));
                 }
                 return bytes;
               }) |
          std::views::join |
          std::ranges::to<ostd::vector<uint8_t>>();
      }

      case OCMD_DATA_TYPE_USER_DEFINED: {
        /// \todo implement user-defined type parsing
        return {};
      }
      default:
        return {};
    }
  }

}  // namespace other