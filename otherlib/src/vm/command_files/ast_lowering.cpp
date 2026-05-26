/**
 * \file vm/command_files/ast_lowering.cpp
 **/
#include "vm/command_files/ast_lowering.hpp"

#include "vm/command_files/compiler_error.hpp"
#include "vm/command_files/opcode_builder.hpp"

namespace other {

  normalized_operand normalize_argument(const canonical_opcode cat_and_type, size_t idx, const raw_instruction::argument& arg) {
    switch (arg.type) {
      case TOKEN_TYPE_ADDRESS:
        return { .kind = operand_kind::ADDRESS_U16, .address = arg.value.value() };
      case TOKEN_TYPE_INTEGER_LITERAL:
        return { .kind = operand_kind::INTEGER_LITERAL, .constant = arg.value.value() };
      case TOKEN_TYPE_FLOATING_POINT_LITERAL:
        return { .kind = operand_kind::FLOAT_LITERAL, .bytes = arg.raw_data };
      case TOKEN_TYPE_STRING_LITERAL:
        return { .kind = operand_kind::STRING_LITERAL, .bytes = arg.raw_data };
      case TOKEN_TYPE_LABEL:
        return { .kind = operand_kind::CODE_LABEL, .symbol = arg.raw_txt };
      case TOKEN_TYPE_IDENTIFIER:
        if (arg.raw_txt.contains('.')) {
          return { .kind = operand_kind::DATA_SYMBOL, .symbol = arg.raw_txt };
        } else {
          return { .kind = operand_kind::CODE_LABEL, .symbol = arg.raw_txt };
        }
      default:
        if (arg.type >= TOKEN_TYPE_KW_R0 && arg.type <= TOKEN_TYPE_KW_RFLAG) {
          return {
            .kind = operand_kind::REGISTER_REF,
            .reg = static_cast<uint8_t>(arg.value.value()),
          };
        }
        return {};
    }
  }

}  // namespace other