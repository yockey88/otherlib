/**
 * \file ocmd_compiler.hpp
 */
#ifndef OTHERLIB_SRC_VM_COMMAND_FILES_OCMD_COMPILER_HPP
#define OTHERLIB_SRC_VM_COMMAND_FILES_OCMD_COMPILER_HPP

#include <string_view>
#include <vector>

#include "vm/command_files/ocmd_ir.hpp"

namespace other {

  class ocmd_compiler {
   public:
    ocmd_compiler(const ocmd_ir& ir)
        : ir(ir) {}
    ~ocmd_compiler() = default;

    std::vector<uint8_t> compile();

   private:
    const ocmd_ir& ir;

    std::vector<uint32_t> emitted_opcodes;
    std::vector<uint8_t> emitted_data;

    inline void emit_opcode(uint32_t opcode) {
      emitted_opcodes.push_back(opcode);
    }

    void compile_to_byte_code();
    void compile_to_binary();
  };

}  // namespace other

#endif  // OTHERLIB_SRC_VM_COMMAND_FILES_OCMD_COMPILER_HPP