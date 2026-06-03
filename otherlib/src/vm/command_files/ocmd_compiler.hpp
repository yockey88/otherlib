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

  class diagnostic_engine;

  class ocmd_compiler {
   public:
    ocmd_compiler() = default;
    ocmd_compiler(const ocmd_ir& ir)
        : ir(ir) {
    }
    ~ocmd_compiler() = default;

    ocmd_program compile(const std::string_view source, scope<ocmd_code_generator> generator, diagnostic_engine* diag);
    ocmd_program compile(scope<ocmd_code_generator> generator, diagnostic_engine* diag);

   private:
    vm_version version = {};

    diagnostic_engine* diagnostics;
    ocmd_ir ir;
  };

}  // namespace other

#endif  // OTHERLIB_SRC_VM_COMMAND_FILES_OCMD_COMPILER_HPP