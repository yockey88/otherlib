/**
 * \file vm/instruction.hpp
 **/
#ifndef OTHERLIB_VM_INSTRUCTION_HPP
#define OTHERLIB_VM_INSTRUCTION_HPP

#include "vm/command_files/code_block.hpp"
#include "vm/operand.hpp"
#include "vm/vm_version.hpp"

namespace other {

  struct canonical_instruction {
    constexpr static inline size_t kMaxOperands = 3;
    canonical_opcode opcode = canonical_opcode::RET_OP;

    /// instr x, y, z
    // x and y are inputs, z is output
    canonical_operand param[kMaxOperands] = {};

    uint32_t deduced_empty_opcode = 0x00000000;
    vm_version source_version = { 0, 0, 0 };

    static canonical_operand lower_argument(const normalized_operand& operand);
    static uint32_t opcode_parity(canonical_opcode canon_opcode);
  };

}  // namespace other

#endif  //   OTHERLIB_VM_INSTRUCTION_HPP