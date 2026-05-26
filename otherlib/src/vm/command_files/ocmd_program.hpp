/**
 * \file vm/command_files/ocmd_program.hpp
 **/
#ifndef OTHERLIB_VM_COMMAND_FILES_OCMD_PROGRAM_HPP
#define OTHERLIB_VM_COMMAND_FILES_OCMD_PROGRAM_HPP

#include "vm/command_files/code_generator.hpp"
#include "vm/command_files/compiler_definition.hpp"

namespace other {

  struct compiled_code_block {
    std::string name;
    bool is_entry_point = false;
    lowering_artifact artifact;
  };

  struct compiled_data_section {
    struct field {
      std::string name;
      uint32_t offset;
      uint32_t size;
    };
    std::string name;
    std::vector<field> fields;
    std::vector<uint8_t> data;
  };

  struct ocmd_program {
    vm_version compiler_version;
    std::vector<compiler_definition> definitions;
    std::vector<compiled_code_block> compiled_blocks;
    std::vector<compiled_data_section> compiled_data_sections;

    bool valid = false;
  };

}  // namespace other

#endif  // OTHERLIB_VM_COMMAND_FILES_OCMD_PROGRAM_HPP