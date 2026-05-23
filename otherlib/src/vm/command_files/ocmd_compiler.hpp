/**
 * \file ocmd_compiler.hpp
 */
#ifndef OTHERLIB_SRC_VM_COMMAND_FILES_OCMD_COMPILER_HPP
#define OTHERLIB_SRC_VM_COMMAND_FILES_OCMD_COMPILER_HPP

#include <string_view>
#include <vector>

namespace other {

  class ocmd_compiler {
   public:
    ocmd_compiler() = default;
    ~ocmd_compiler() = default;

    // static std::vector<uint8_t> parse_and_assemble_assembly_file(const filepath& path);
    static std::vector<uint8_t> compile_single_translation_unit(const std::string_view source_code);
  };

}  // namespace other

#endif  // OTHERLIB_SRC_VM_COMMAND_FILES_OCMD_COMPILER_HPP