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

  /**
   * \note: opcodes are 32 bit integers of the form --xxxxxx where -- is the category/type, and xxxxxx are the opcode parameters which can take the form:
   * - xx0000 for opcodes with one register operand
   * - xxyy00 for opcodes with two register operands
   * - xxyyzz for opcodes with three register operands
   * - nnnnxx for opcodes with one 16-bit immediate/address operand and one register operand
   * - nnnn00 for opcodes with one 16-bit immediate/address operand and no register operand
   * - xxnnnn for opcodes with one 16-bit immediate/address operand and one register operand
   * - 00nnnn for opcodes with one 16-bit immediate/address operand and no register operand
   *
   * normalized_operand usage:
   *  - reg1 operand is for first register (xx)
   *  - reg2 operand is for second register (yy) if present
   *  - reg3 operand is for third register (zz) if present
   *  - value1 is for first 16-bit immediate/address operand (nnnn) if present
   *  - value2 is for second 16-bit immediate/address operand (nnnn) if present
   */
  // type to make parsing easier
  struct normalized_operand {
    operand_kind kind = operand_kind::INVALID;
    vm_type type = VM_TYPE_VOID;

    opt<uint8_t> reg = std::nullopt;
    opt<std::string> symbol = std::nullopt;
    opt<uint16_t> constant = std::nullopt;
    opt<uint16_t> address = std::nullopt;

    std::vector<uint8_t> bytes = {};
  };

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

    constexpr static inline size_t kMaxArguments = 3;

    canonical_opcode opcode = canonical_opcode::INVALID_OP;
    std::vector<argument> arguments = {};
  };

  struct jump_label {
    std::string name = "";
    uint16_t section_address = 0;
  };

  struct code_block {
    std::string name = "";
    natural_t name_hash = 0;
    std::vector<raw_instruction> instructions = {};
    std::vector<jump_label> jump_labels = {};
  };

  normalized_operand normalize_argument(const canonical_opcode cat_and_type, size_t idx, const raw_instruction::argument& arg);

}  // namespace other

#endif  // OTHERLIB_VM_COMMAND_FILES_CODE_BLOCK_HPP