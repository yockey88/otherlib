/**
 * \file vm/command_files/code_block.hpp
 **/
#ifndef OTHERLIB_VM_COMMAND_FILES_CODE_BLOCK_HPP
#define OTHERLIB_VM_COMMAND_FILES_CODE_BLOCK_HPP

#include <cstdint>
#include <vector>

#include "core/defines.hpp"

#include "vm/command_files/token.hpp"
#include "vm/opcode.hpp"

namespace other {

  struct raw_instruction {
    struct argument {
      std::string raw_txt;
      token_type type = TOKEN_TYPE_INVALID;

      /// either address, constant, or register index (all 16 bits or 8 bits)
      opt<uint16_t> value = 0;
      /// for raw data (e.g., string literals, floating-point literals)
      std::vector<uint8_t> raw_data = {};

      static argument from_token(const token& tok);
    };
    uint32_t category_and_type = 0;
    std::vector<argument> arguments = {};

    static uint32_t get_opcode(uint32_t category_and_type, const std::vector<argument>& arguments);
    static std::vector<token> get_argument_tokens_for_instruction(const uint32_t category_and_type, const std::vector<token>& arg_tokens);
  };

  struct code_block {
    std::string name = "";
    natural_t name_hash = 0;
    std::vector<raw_instruction> instructions = {};
  };

}  // namespace other

#endif  // OTHERLIB_VM_COMMAND_FILES_CODE_BLOCK_HPP