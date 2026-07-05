/**
 * \file vm/command_files/opcode_builder.hpp
 **/
#ifndef OTHERLIB_VM_COMMAND_FILES_OPCODE_BUILDER_HPP
#define OTHERLIB_VM_COMMAND_FILES_OPCODE_BUILDER_HPP

#include "vm/command_files/code_generator.hpp"
#include "vm/instruction.hpp"
#include "vm/vm_version.hpp"

namespace other {

  enum class scratch_policy : uint8_t {
    NONE = 0,
    PREFER_RF,
    RESERVE_FROM_CONTEXT,
  };

  class opcode_builder {
   public:
    opcode_builder(const vm_version& target_vm_version)
        : target_vm_version(target_vm_version) {}
    ~opcode_builder() = default;

    void lower_raw_instruction(const raw_instruction& instr);
    void add_jump_label(const std::string& name, uint16_t section_address);

    uint8_t acquire_scratch_register(scratch_policy policy);
    void reset_scratch_registers();

    void add_symbol_fixup(uint32_t opcode_index, const std::string& symbol);

    lowering_artifact finalize(ocmd_code_generator& generator) const;

    std::span<const canonical_instruction> get_emitted_instructions() const {
      return emitted_instructions;
    }

   private:
    uint32_t next_literal_id = 0;
    uint8_t next_scratch_register = 0;

    vm_version target_vm_version;

    ostd::vector<fixup_handle> jump_labels;
    ostd::vector<canonical_instruction> emitted_instructions;

    void emit(const canonical_instruction& instr);
  };

}  // namespace other

#endif  // OTHERLIB_VM_COMMAND_FILES_OPCODE_BUILDER_HPP