/**
 * \file vm/command_files/opcode_builder.cpp
 **/
#include "vm/command_files/opcode_builder.hpp"

#include "vm/command_files/compiler_error.hpp"
#include "vm/register.hpp"

namespace other {

  void opcode_builder::lower_raw_instruction(const raw_instruction& instr) {
    if (instr.arguments.size() > 3) {
      throw ocmd_lowering_error("Too many arguments in raw instruction");
    }

    canonical_instruction canon_instr{ .opcode = instr.opcode };

    std::vector<normalized_operand> normalized_operands;
    for (size_t i = 0; i < instr.arguments.size(); ++i) {
      normalized_operands.push_back(normalize_argument(canon_instr.opcode, i, instr.arguments[i]));
    }

    for (size_t a = 0; a < raw_instruction::kMaxArguments; ++a) {
      if (instr.arguments.size() > a) {
        canon_instr.param[a] = canonical_instruction::lower_argument(normalized_operands[a]);
      } else {
        break;
      }
    }

    emit(canon_instr);
  }

  void opcode_builder::add_jump_label(const std::string& name, uint16_t section_address) {
    jump_labels.push_back(fixup_handle{ .symbol_name = name, .opcode_index = section_address });
  }

  uint8_t opcode_builder::acquire_scratch_register(scratch_policy policy) {
    if (policy == scratch_policy::PREFER_RF) {
      return vm_register_idx::VM_RF;
    } else {
      if (next_scratch_register >= vm_register::kNumRegisters) {
        throw ocmd_lowering_error("Out of scratch registers");
      }
      return next_scratch_register++;
    }
  }

  void opcode_builder::reset_scratch_registers() {
    next_scratch_register = vm_register_idx::VM_R0;
  }

  lowering_artifact opcode_builder::finalize(ocmd_code_generator& generator) const {
    lowering_artifact artifact{};
    for (const auto& instr : emitted_instructions) {
      generator.encode(instr, artifact);
    }
    artifact.jump_labels = jump_labels;
    return artifact;
  }

  void opcode_builder::emit(const canonical_instruction& instr) {
    emitted_instructions.push_back(instr);
  }

}  // namespace other