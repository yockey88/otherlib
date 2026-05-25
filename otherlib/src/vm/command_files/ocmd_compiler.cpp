/**
 * \file vm/command_files/ocmd_compiler.cpp
 **/
#include "vm/command_files/ocmd_compiler.hpp"

#include "vm/opcode.hpp"

namespace other {

  std::vector<uint8_t> ocmd_compiler::compile() {
    compile_to_byte_code();
    compile_to_binary();
    return emitted_data;
  }

  void ocmd_compiler::compile_to_byte_code() {
    for (const auto& code_blk_ir : ir.code_blocks) {
      for (const auto& instr_ir : code_blk_ir.instructions) {
        instruction instr;
        instr.opcode = instr_ir.category_and_type;
        // set opcode fields based on instruction arguments
      }
    }
  }

  void ocmd_compiler::compile_to_binary() {}

}  // namespace other