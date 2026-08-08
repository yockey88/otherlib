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
#include "vm/operand.hpp"

namespace other {

  /** \note opcodes are 32-bit ints: --xxxxxx (category + up to 3 reg bytes, or a 16-bit
   *   immediate/address + optional register) **/
  // type to make parsing easier
  struct normalized_operand {
    operand_kind kind = operand_kind::INVALID;
    vm_type type = VM_TYPE_VOID;
    bool indirect = false;

    opt<uint8_t> reg = std::nullopt;
    opt<std::string> symbol = std::nullopt;
    opt<uint16_t> constant = std::nullopt;
    opt<uint16_t> address = std::nullopt;

    ostd::vector<uint8_t> bytes = {};
  };

  struct raw_instruction {
    struct argument {
      std::string raw_txt;
      token_type type = TOKEN_TYPE_INVALID;
      bool indirect = false;

      /// either address, constant, or register index (all 16 bits or 8 bits)
      opt<uint16_t> value = 0;
      /// for raw data (e.g., string literals, floating-point literals)
      ostd::vector<uint8_t> raw_data = {};

      static argument from_token(const token& tok);
    };

    constexpr static inline size_t kMaxArguments = 3;

    canonical_opcode opcode = canonical_opcode::INVALID_OP;
    ostd::vector<argument> arguments = {};
  };

  struct jump_label {
    std::string name = "";
    uint16_t instruction_index = 0;
  };

  struct code_block {
    std::string name = "";
    natural_t name_hash = 0;
    ostd::vector<raw_instruction> instructions = {};
    ostd::vector<jump_label> jump_labels = {};
  };

  normalized_operand normalize_argument(const canonical_opcode cat_and_type, size_t idx, const raw_instruction::argument& arg);

}  // namespace other

#endif  // OTHERLIB_VM_COMMAND_FILES_CODE_BLOCK_HPP