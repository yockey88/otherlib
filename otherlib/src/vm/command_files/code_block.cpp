/**
 * \file vm/command_files/code_block.cpp
 **/
#include "vm/command_files/code_block.hpp"

#include "core/enum_formatter.hpp"
#include "core/logger.hpp"

#include "vm/opcode.hpp"
#include "vm/register.hpp"

#include "token.hpp"

namespace other {
  namespace {

    uint8_t instruction_register_index_from_token(const token& tok) {
      if (tok.type < TOKEN_TYPE_KW_R0 || tok.type > TOKEN_TYPE_KW_RFLAG) {
        throw std::runtime_error("Token is not a register: " + tok.text);
      }

      if ("r0" == tok.text) return vm_register_idx::VM_R0;
      if ("r1" == tok.text) return vm_register_idx::VM_R1;
      if ("r2" == tok.text) return vm_register_idx::VM_R2;
      if ("r3" == tok.text) return vm_register_idx::VM_R3;
      if ("r4" == tok.text) return vm_register_idx::VM_R4;
      if ("r5" == tok.text) return vm_register_idx::VM_R5;
      if ("r6" == tok.text) return vm_register_idx::VM_R6;
      if ("r7" == tok.text) return vm_register_idx::VM_R7;
      if ("r8" == tok.text) return vm_register_idx::VM_R8;
      if ("r9" == tok.text) return vm_register_idx::VM_R9;
      if ("ra" == tok.text) return vm_register_idx::VM_RA;
      if ("rb" == tok.text) return vm_register_idx::VM_RB;
      if ("rc" == tok.text) return vm_register_idx::VM_RC;
      if ("rd" == tok.text) return vm_register_idx::VM_RD;
      if ("re" == tok.text) return vm_register_idx::VM_RE;
      if ("rf" == tok.text) return vm_register_idx::VM_RF;
      if ("rflag" == tok.text) return vm_register_idx::VM_RFLAG;

      throw std::runtime_error("Invalid register token: " + tok.text);
    }

  }  // namespace

  raw_instruction::argument raw_instruction::argument::from_token(const token& tok) {
    auto arg = raw_instruction::argument{
      .raw_txt = tok.text,
      .type = tok.type,
    };

    if (tok.type >= TOKEN_TYPE_KW_R0 && tok.type <= TOKEN_TYPE_KW_RFLAG) {
      arg.value = instruction_register_index_from_token(tok);
    } else if (tok.type == TOKEN_TYPE_ADDRESS) {
      arg.value = static_cast<uint16_t>(std::stoul(tok.text, nullptr, 16));
    } else if (tok.type == TOKEN_TYPE_INTEGER_LITERAL) {
      arg.value = static_cast<uint16_t>(std::stoul(tok.text, nullptr, 10));
    } else if (tok.type == TOKEN_TYPE_FLOATING_POINT_LITERAL) {
      float fvalue = std::stof(tok.text);
      arg.raw_data.resize(sizeof(float));
      std::memcpy(arg.raw_data.data(), &fvalue, sizeof(float));
    } else if (tok.type == TOKEN_TYPE_STRING_LITERAL) {
      arg.raw_data = std::vector<uint8_t>(tok.text.begin(), tok.text.end());
    } else if (tok.type == TOKEN_TYPE_LABEL) {
      /// mark as unresolved address for now
      arg.value = 0xFFFF;
    }
    return arg;
  }

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