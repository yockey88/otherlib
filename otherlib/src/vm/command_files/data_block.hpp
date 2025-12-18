/**
 * \file vm/command_files/data_block.hpp
 **/
#ifndef OTHERLIB_VM_COMMAND_FILES_DATA_BLOCK_HPP
#define OTHERLIB_VM_COMMAND_FILES_DATA_BLOCK_HPP

#include <cstdint>
#include <vector>

#include "core/defines.hpp"

#include "vm/command_files/token.hpp"

namespace other {

  enum data_type {
    OCMD_DATA_TYPE_I8,
    OCMD_DATA_TYPE_U8,
    OCMD_DATA_TYPE_I16,
    OCMD_DATA_TYPE_U16,
    OCMD_DATA_TYPE_I32,
    OCMD_DATA_TYPE_U32,
    OCMD_DATA_TYPE_I64,
    OCMD_DATA_TYPE_U64,
    OCMD_DATA_TYPE_F32,
    OCMD_DATA_TYPE_F64,
    OCMD_DATA_TYPE_STRING,
    OCMD_DATA_TYPE_BLOB,
    OCMD_DATA_TYPE_USER_DEFINED,

    OCMD_DATA_TYPE_INVALID
  };

  struct data_object {
    std::string name = "";
    token value_token;

    data_type type = OCMD_DATA_TYPE_INVALID;
    uint16_t address = 0;
    std::vector<uint8_t> data = {};

    static data_type data_type_from_label(const std::string& label);
    static data_type deduce_data_type_from_tokens(const std::vector<token>& tokens);
    static std::vector<uint8_t> data_from_token_and_type(const token& value_token, data_type type);
  };

  struct data_block {
    std::string name = "";
    natural_t name_hash = 0;

    std::vector<data_object> objects = {};
  };

}  // namespace other

#endif  // OTHERLIB_VM_COMMAND_FILES_DATA_BLOCK_HPP