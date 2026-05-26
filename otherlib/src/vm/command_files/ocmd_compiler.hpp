/**
 * \file ocmd_compiler.hpp
 */
#ifndef OTHERLIB_SRC_VM_COMMAND_FILES_OCMD_COMPILER_HPP
#define OTHERLIB_SRC_VM_COMMAND_FILES_OCMD_COMPILER_HPP

#include <string_view>
#include <vector>

#include "core/scope.hpp"

#include "vm/command_files/ocmd_ir.hpp"
#include "vm/command_files/ocmd_program.hpp"

namespace other {

  class ocmd_compiler {
    OTHER_ENVIRONMENT_INTERFACE("VM", "OcmdCompiler");

   public:
    ocmd_compiler(const ocmd_ir& ir)
        : ir(ir) {
    }
    virtual ~ocmd_compiler() = default;

    virtual ocmd_program compile(scope<ocmd_code_generator> generator);

   private:
    const ocmd_ir ir;

    canonical_instruction lower_to_canonical_instruction(const raw_instruction& instr);
  };

}  // namespace other

#endif  // OTHERLIB_SRC_VM_COMMAND_FILES_OCMD_COMPILER_HPP