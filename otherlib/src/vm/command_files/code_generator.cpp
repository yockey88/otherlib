/**
 * \file vm/command_files/code_generator.cpp
 **/
#include "vm/command_files/code_generator.hpp"

#include "vm/command_files/code_block.hpp"
#include "vm/command_files/compiler_error.hpp"

namespace other {

  void ocmd_code_generator::encode(const canonical_instruction& instr, lowering_artifact& artifact) {
    switch (instr.opcode) {
      case canonical_opcode::STOPDEV_OP: return encode_stopdev(instr, artifact);
      case canonical_opcode::DUMP_OP: return encode_dump(instr, artifact);
      case canonical_opcode::WRITE_OP: return encode_write(instr, artifact);
      case canonical_opcode::SET_OP: return encode_set(instr, artifact);
      case canonical_opcode::CMP_OP: return encode_cmp(instr, artifact);
      case canonical_opcode::CMPGT_OP: return encode_cmpgt(instr, artifact);
      case canonical_opcode::CMPLT_OP: return encode_cmplt(instr, artifact);
      case canonical_opcode::AND_OP: return encode_and(instr, artifact);
      case canonical_opcode::OR_OP: return encode_or(instr, artifact);
      case canonical_opcode::XOR_OP: return encode_xor(instr, artifact);
      case canonical_opcode::LSHIFT_OP: return encode_lshift(instr, artifact);
      case canonical_opcode::RSHIFT_OP: return encode_rshift(instr, artifact);
      case canonical_opcode::GOTO_OP: return encode_goto(instr, artifact);
      case canonical_opcode::JE_OP: return encode_je(instr, artifact);
      case canonical_opcode::JNE_OP: return encode_jne(instr, artifact);
      case canonical_opcode::CALL_OP: return encode_call(instr, artifact);
      case canonical_opcode::SYSCALL_OP: return encode_syscall(instr, artifact);
      case canonical_opcode::RET_OP: return encode_return(instr, artifact);
      case canonical_opcode::ADD_OP: return encode_add(instr, artifact);
      case canonical_opcode::SUB_OP: return encode_sub(instr, artifact);
      case canonical_opcode::MUL_OP: return encode_mul(instr, artifact);
      case canonical_opcode::DIV_OP: return encode_div(instr, artifact);
      case canonical_opcode::MOD_OP: return encode_mod(instr, artifact);
      default:
        throw ocmd_lowering_error("selector v1 does not support this canonical opcode");
    }
  }

  void ocmd_code_generator::add_symbol_fixup(uint32_t opcode_index, const std::string& symbol, lowering_artifact& artifact) {
    artifact.unresolved_labels.push_back({ .symbol_name = symbol, .opcode_index = opcode_index });
  }

  void ocmd_code_generator::emit_instruction(const instruction& instr, lowering_artifact& artifact) {
    artifact.machine_instructions.push_back(instr);
  }

}  // namespace other