/**
 * \file vm/instruction.cpp
 * */
#include "vm/instruction.hpp"

#include "core/enum_formatter.hpp"

#include "vm/command_files/compiler_error.hpp"

namespace other {

  canonical_operand canonical_instruction::lower_argument(const normalized_operand& operand) {
    switch (operand.kind) {
      case operand_kind::ADDRESS_U16:
        if (!operand.address.has_value()) {
          throw ocmd_lowering_error("ADDRESS_U16 operand is missing address value");
        }
        return { .kind = operand_kind::ADDRESS_U16, .type = operand.type, .val = operand.address.value() };
      case operand_kind::INTEGER_LITERAL:
        if (!operand.constant.has_value()) {
          throw ocmd_lowering_error("INTEGER_LITERAL operand is missing constant value");
        }
        return { .kind = operand_kind::IMMEDIATE_U16, .type = operand.type, .val = operand.constant.value() };
      case operand_kind::FLOAT_LITERAL:
        return { .kind = operand_kind::FLOAT_LITERAL, .type = operand.type, .bytes = operand.bytes };
      case operand_kind::STRING_LITERAL:
        return { .kind = operand_kind::STRING_LITERAL, .type = operand.type, .bytes = operand.bytes };
      case operand_kind::CODE_LABEL:
        if (!operand.symbol.has_value()) {
          throw ocmd_lowering_error("CODE_LABEL operand is missing symbol");
        }
        return { .kind = operand_kind::CODE_LABEL, .type = operand.type, .symbol = operand.symbol.value() };
      case operand_kind::DATA_SYMBOL:
        if (!operand.symbol.has_value()) {
          throw ocmd_lowering_error("DATA_SYMBOL operand is missing symbol");
        }
        return { .kind = operand_kind::DATA_SYMBOL, .type = operand.type, .symbol = operand.symbol.value() };
      case operand_kind::REGISTER_REF:
        if (!operand.reg.has_value()) {
          throw ocmd_lowering_error("REGISTER_REF operand is missing register index");
        }
        return { .kind = operand_kind::REGISTER_REF, .type = operand.type, .reg = operand.reg.value() };
      default:
        throw ocmd_lowering_error(std::format("Invalid operand kind during lowering: {}", operand.kind));
    }
  }

  uint32_t canonical_instruction::opcode_parity(canonical_opcode canon_opcode) {
    /// return MAX instruction parity
    switch (canon_opcode) {
      // 0
      case canonical_opcode::STOPDEV_OP: return 0;
      case canonical_opcode::DUMP_OP: return 2;
      case canonical_opcode::VIEW_STATE_OP: return 0;
      // 1
      case canonical_opcode::WRITE_OP: return 2;
      case canonical_opcode::SET_OP: return 2;
      case canonical_opcode::CMP_OP: return 3;
      case canonical_opcode::CMPGT_OP: return 3;
      case canonical_opcode::CMPLT_OP: return 3;
      case canonical_opcode::AND_OP: return 3;
      case canonical_opcode::OR_OP: return 3;
      case canonical_opcode::XOR_OP: return 3;
      case canonical_opcode::LSHIFT_OP: return 2;
      case canonical_opcode::RSHIFT_OP: return 2;
      case canonical_opcode::MOV_OP: return 2;
      // 2
      case canonical_opcode::GOTO_OP: return 1;
      case canonical_opcode::JE_OP: return 1;
      case canonical_opcode::JNE_OP: return 1;
      case canonical_opcode::CALL_OP: return 1;
      case canonical_opcode::RET_OP: return 1;
      // syscall and invoke can take the syscall id and an
      // address pointing to a defined argument-structure
      // this is in addition to the natural calling convention of passing arguments in registers,
      // so we give it a parity of 2 to allow for both styles
      case canonical_opcode::SYSCALL_OP: return 2;
      case canonical_opcode::INVOKE_OP: return 2;
      // 3
      case canonical_opcode::ADD_OP: return 2;
      case canonical_opcode::SUB_OP: return 2;
      case canonical_opcode::MUL_OP: return 2;
      case canonical_opcode::DIV_OP: return 2;
      case canonical_opcode::MOD_OP: return 2;
      default:
        throw ocmd_lowering_error(std::format("Invalid canonical opcode for parity calculation: {}", canon_opcode));
    }
    OTHER_ASSERT(false, "Unreachable code in opcode_parity");
  }

}  // namespace other