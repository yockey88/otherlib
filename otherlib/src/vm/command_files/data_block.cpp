/**
 * \file vm/command_files/data_block.cpp
 **/
#include "vm/command_files/data_block.hpp"

#include <cstdint>

#include "token.hpp"

namespace other {
  namespace {

    template <typename T>
    std::vector<uint8_t> raw_data_from_value(const T& value) {
      std::vector<uint8_t> data(sizeof(T));
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

  data_type data_object::deduce_data_type_from_tokens(const std::vector<token>& value_tokens) {
    /// check if blob is of the form DE AD BE EF which shows as either all HEX_LITERAL or mix of HEX_LITERAL and INTEGER_LITERAL, but
    /// we don't want to misinterpret integer literals like "123456" as blob, we will interpret any thing more than two 123 456 as blob or 0x123456 as blob,
    /// but not single integer literals

    if (std::ranges::all_of(value_tokens, [](const token& tok) { return tok.type == TOKEN_TYPE_HEX_LITERAL; }) ||
        (value_tokens.size() > 1 && std::ranges::all_of(value_tokens, [](const token& tok) { return tok.type == TOKEN_TYPE_HEX_LITERAL || tok.type == TOKEN_TYPE_INTEGER_LITERAL; }))) {
      return data_type::OCMD_DATA_TYPE_BLOB;
    } else if (std::ranges::all_of(value_tokens, [](const token& tok) { return tok.type == TOKEN_TYPE_STRING_LITERAL; })) {
      return data_type::OCMD_DATA_TYPE_STRING;
    }
    /// default to int32 for integer literals, can fix later
    else if (std::ranges::all_of(value_tokens, [](const token& tok) { return tok.type == TOKEN_TYPE_INTEGER_LITERAL; })) {
      return data_type::OCMD_DATA_TYPE_I32;
    }
    /// default to float32 for floating-point literals, can fix later
    else if (std::ranges::all_of(value_tokens, [](const token& tok) { return tok.type == TOKEN_TYPE_FLOATING_POINT_LITERAL; })) {
      return data_type::OCMD_DATA_TYPE_F32;
    } else {
      return data_type::OCMD_DATA_TYPE_INVALID;
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

  std::vector<uint8_t> data_object::data_from_token_and_type(const token& value_token, data_type type) {
    switch (type) {
      case OCMD_DATA_TYPE_I8: {
        int8_t val = static_cast<int8_t>(std::stoll(value_token.text));
        return raw_data_from_value<int8_t>(val);
      }
      case OCMD_DATA_TYPE_U8: {
        uint8_t val = static_cast<uint8_t>(std::stoull(value_token.text));
        return raw_data_from_value<uint8_t>(val);
      }
      case OCMD_DATA_TYPE_I16: {
        int16_t val = static_cast<int16_t>(std::stoll(value_token.text));
        return raw_data_from_value<int16_t>(val);
      }
      case OCMD_DATA_TYPE_U16: {
        uint16_t val = static_cast<uint16_t>(std::stoull(value_token.text));
        return raw_data_from_value<uint16_t>(val);
      }
      case OCMD_DATA_TYPE_I32: {
        int32_t val = static_cast<int32_t>(std::stoll(value_token.text));
        return raw_data_from_value<int32_t>(val);
      }
      case OCMD_DATA_TYPE_U32: {
        uint32_t val = static_cast<uint32_t>(std::stoull(value_token.text));
        return raw_data_from_value<uint32_t>(val);
      }
      case OCMD_DATA_TYPE_F32: return raw_data_from_value<float>(std::stof(value_token.text));
      case OCMD_DATA_TYPE_F64: return raw_data_from_value<double>(std::stod(value_token.text));
      case OCMD_DATA_TYPE_STRING: return std::vector<uint8_t>(value_token.text.begin(), value_token.text.end());

      case OCMD_DATA_TYPE_BLOB: {
        std::string raw_txt = value_token.text;
        if (raw_txt.starts_with("0x")) {
          raw_txt = value_token.text.substr(2);
        }

        return raw_txt |
          /// split data blob string on spaces and then trim white space and then filter if empty
          std::views::split(' ') | std::views::transform([](auto&& byte_str_view) { return trim_beginning_and_end(byte_str_view | std::ranges::to<std::string>()); }) |
          std::views::filter([](auto&& byte_str_view) { return !byte_str_view.empty(); }) |
          /// turn each piece into a vector of uint8_t depending on how many bytes are in the string then flatten and collect
          std::views::transform([](const std::string& byte_str) {
                 size_t num_bytes = byte_str.size() / 2 + (byte_str.size() % 2 != 0 ? 1 : 0);
                 std::vector<uint8_t> bytes;
                 for (size_t i = 0; i < num_bytes; ++i) {
                   std::string byte_hex = byte_str.substr(i * 2, 2);
                   uint8_t byte = static_cast<uint8_t>(std::stoul(byte_hex, nullptr, 16));
                   bytes.push_back(byte);
                 }
                 return bytes;
               }) |
          std::views::join |
          std::ranges::to<std::vector>();
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