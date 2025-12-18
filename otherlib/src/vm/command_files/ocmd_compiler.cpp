/**
 * \file vm/command_files/ocmd_compiler.cpp
 **/
#include "vm/command_files/ocmd_compiler.hpp"

#include "vm/command_files/assembler.hpp"
#include "vm/command_files/lexer.hpp"
#include "vm/command_files/linker.hpp"
#include "vm/command_files/parser.hpp"

namespace other {

  std::vector<uint8_t> ocmd_compiler::compile_single_translation_unit(const std::string_view source_code) {
    ocmd_lexer lexer{ source_code };
    const auto tokens = lexer.tokenize();
    if (tokens.empty()) {
      return {};
    }

    ocmd_parser parser{ tokens };
    const auto ir = parser.parse();
    if (ir.code_blocks.empty() || ir.data_blocks.empty()) {
      return {};
    }

    ocmd_assembler assembler{ ir };
    const auto object_code = assembler.assemble();
    if (object_code.code.empty() || object_code.data.empty()) {
      return {};
    }

    ocmd_linker linker{ object_code };
    return linker.link();
  }

}  // namespace other