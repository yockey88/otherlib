/**
 * \file vm/code_generator_000.hpp
 **/
#ifndef OTHERLIB_VM_CODE_GENERATOR_000_HPP
#define OTHERLIB_VM_CODE_GENERATOR_000_HPP

#include "vm/command_files/code_generator.hpp"

namespace other {

  class code_generator_000 : public ocmd_code_generator {
   public:
    code_generator_000() = default;
    ~code_generator_000() = default;

    vm_version target_version() const override { return { 0, 0, 0 }; }

   private:
    void encode_stopdev(const canonical_instruction& instr, lowering_artifact& artifact) override;
    void encode_dump(const canonical_instruction& instr, lowering_artifact& artifact) override;
    void encode_view_state(const canonical_instruction& instr, lowering_artifact& artifact) override;

    void encode_mov(const canonical_instruction& instr, lowering_artifact& artifact) override;
    void encode_write(const canonical_instruction& instr, lowering_artifact& artifact) override;
    void encode_set(const canonical_instruction& instr, lowering_artifact& artifact) override;

    void encode_goto(const canonical_instruction& instr, lowering_artifact& artifact) override;
    void encode_je(const canonical_instruction& instr, lowering_artifact& artifact) override;
    void encode_jne(const canonical_instruction& instr, lowering_artifact& artifact) override;
    void encode_call(const canonical_instruction& instr, lowering_artifact& artifact) override;
    void encode_return(const canonical_instruction& instr, lowering_artifact& artifact) override;
    void encode_syscall(const canonical_instruction& instr, lowering_artifact& artifact) override;
    void encode_invoke(const canonical_instruction& instr, lowering_artifact& artifact) override;

    void encode_add(const canonical_instruction& instr, lowering_artifact& artifact) override;
    void encode_sub(const canonical_instruction& instr, lowering_artifact& artifact) override;
    void encode_mul(const canonical_instruction& instr, lowering_artifact& artifact) override;
    void encode_div(const canonical_instruction& instr, lowering_artifact& artifact) override;
    void encode_mod(const canonical_instruction& instr, lowering_artifact& artifact) override;

    void encode_cmp(const canonical_instruction& instr, lowering_artifact& artifact) override;
    void encode_cmpgt(const canonical_instruction& instr, lowering_artifact& artifact) override;
    void encode_cmplt(const canonical_instruction& instr, lowering_artifact& artifact) override;
    void encode_and(const canonical_instruction& instr, lowering_artifact& artifact) override;
    void encode_or(const canonical_instruction& instr, lowering_artifact& artifact) override;
    void encode_xor(const canonical_instruction& instr, lowering_artifact& artifact) override;
    void encode_lshift(const canonical_instruction& instr, lowering_artifact& artifact) override;
    void encode_rshift(const canonical_instruction& instr, lowering_artifact& artifact) override;
  };

}  // namespace other

#endif  // OTHERLIB_VM_CODE_GENERATOR_000_HPP