/**
 * \file vm/command_files/code_generator.hpp
 **/
#ifndef OTHERLIB_VM_COMMAND_FILES_CODE_GENERATOR_HPP
#define OTHERLIB_VM_COMMAND_FILES_CODE_GENERATOR_HPP

#include "core/interfaces.hpp"

#include "vm/command_files/ocmd_code.hpp"
#include "vm/instruction.hpp"
#include "vm/vm_version.hpp"

namespace other {

  struct fixup_handle {
    std::string symbol_name = "";
    // if jump label then it holds the local section offset of the label
    natural_t opcode_index = 0;
  };

  struct lowering_artifact {
    std::vector<instruction> machine_instructions = {};
    std::vector<fixup_handle> jump_labels = {};
    std::vector<fixup_handle> unresolved_labels = {};
  };

  class ocmd_code_generator {
    OTHER_ENVIRONMENT_INTERFACE("VM", "CodeGenerator");

   public:
    virtual ~ocmd_code_generator() = default;
    virtual vm_version target_version() const = 0;

    void encode(const canonical_instruction& instr, lowering_artifact& out);

   protected:
    void add_symbol_fixup(uint32_t opcode_index, const std::string& symbol, lowering_artifact& artifact);
    void emit_instruction(const instruction& instr, lowering_artifact& artifact);

    virtual void encode_stopdev(const canonical_instruction& instr, lowering_artifact& artifact) = 0;
    virtual void encode_dump(const canonical_instruction& instr, lowering_artifact& artifact) = 0;
    virtual void encode_view_state(const canonical_instruction& instr, lowering_artifact& artifact) = 0;
    virtual void encode_clear(const canonical_instruction& instr, lowering_artifact& artifact) = 0;

    virtual void encode_write(const canonical_instruction& instr, lowering_artifact& artifact) = 0;
    virtual void encode_set(const canonical_instruction& instr, lowering_artifact& artifact) = 0;
    virtual void encode_cmp(const canonical_instruction& instr, lowering_artifact& artifact) = 0;
    virtual void encode_cmpgt(const canonical_instruction& instr, lowering_artifact& artifact) = 0;
    virtual void encode_cmplt(const canonical_instruction& instr, lowering_artifact& artifact) = 0;
    virtual void encode_and(const canonical_instruction& instr, lowering_artifact& artifact) = 0;
    virtual void encode_or(const canonical_instruction& instr, lowering_artifact& artifact) = 0;
    virtual void encode_xor(const canonical_instruction& instr, lowering_artifact& artifact) = 0;
    virtual void encode_lshift(const canonical_instruction& instr, lowering_artifact& artifact) = 0;
    virtual void encode_rshift(const canonical_instruction& instr, lowering_artifact& artifact) = 0;
    virtual void encode_mov(const canonical_instruction& instr, lowering_artifact& artifact) = 0;

    virtual void encode_goto(const canonical_instruction& instr, lowering_artifact& artifact) = 0;
    virtual void encode_je(const canonical_instruction& instr, lowering_artifact& artifact) = 0;
    virtual void encode_jne(const canonical_instruction& instr, lowering_artifact& artifact) = 0;
    virtual void encode_call(const canonical_instruction& instr, lowering_artifact& artifact) = 0;
    virtual void encode_return(const canonical_instruction& instr, lowering_artifact& artifact) = 0;
    virtual void encode_syscall(const canonical_instruction& instr, lowering_artifact& artifact) = 0;
    virtual void encode_invoke(const canonical_instruction& instr, lowering_artifact& artifact) = 0;

    virtual void encode_add(const canonical_instruction& instr, lowering_artifact& artifact) = 0;
    virtual void encode_sub(const canonical_instruction& instr, lowering_artifact& artifact) = 0;
    virtual void encode_mul(const canonical_instruction& instr, lowering_artifact& artifact) = 0;
    virtual void encode_div(const canonical_instruction& instr, lowering_artifact& artifact) = 0;
    virtual void encode_mod(const canonical_instruction& instr, lowering_artifact& artifact) = 0;
  };

}  // namespace other

#endif  // OTHERLIB_VM_COMMAND_FILES_CODE_GENERATOR_HPP