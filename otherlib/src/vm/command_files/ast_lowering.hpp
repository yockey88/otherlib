/**
 * \file vm/command_files/ast_lowering.hpp
 **/
#ifndef OTHERLIB_VM_COMMAND_FILES_AST_LOWERING_HPP
#define OTHERLIB_VM_COMMAND_FILES_AST_LOWERING_HPP

#include "vm/command_files/code_block.hpp"
#include "vm/operand.hpp"

namespace other {

  class opcode_builder;

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
  // same as canonical_operand but optionals for various fields to help with translation/error handling during lowering
  struct normalized_operand {
    operand_kind kind = operand_kind::INVALID;

    opt<uint8_t> reg = std::nullopt;
    opt<std::string> symbol = std::nullopt;
    opt<uint16_t> constant = std::nullopt;
    opt<uint16_t> address = std::nullopt;

    std::vector<uint8_t> bytes = {};
  };

  normalized_operand normalize_argument(const canonical_opcode cat_and_type, size_t idx, const raw_instruction::argument& arg);

}  // namespace other

#endif  // OTHERLIB_VM_COMMAND_FILES_AST_LOWERING_HPP