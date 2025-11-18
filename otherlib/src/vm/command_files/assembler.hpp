/**
 * \file vm/command_files/assembler.hpp
 **/
#ifndef OTHERLIB_VM_COMMAND_FILES_ASSEMBLER_HPP
#define OTHERLIB_VM_COMMAND_FILES_ASSEMBLER_HPP

#include <cstdint>

#include "core/defines.hpp"

#include "vm/command_files/parser.hpp"

#include "code_block.hpp"

namespace other {

  struct ocmd_assembled_code {
    struct unresolved_label {
      uint16_t address = 0;
      std::string label_name = "";
    };
    struct data_object_ptr {
      std::string name = "";
      natural_t offset = 0;
      natural_t size = 0;
    };

    struct section_bound_ptr {
      std::string name = "";
      natural_t offset = 0;
      natural_t size = 0;
    };

    natural_t num_instructions = 0;

    std::vector<uint8_t> code = {};
    std::vector<uint8_t> data = {};

    std::vector<section_bound_ptr> code_section_bounds = {};
    std::vector<unresolved_label> unresolved_labels = {};
    std::vector<data_object_ptr> data_object_ptrs = {};
  };

  class ocmd_assembler {
   public:
    ocmd_assembler(const ocmd_ir& ir)
        : ir(ir) {}
    ~ocmd_assembler() = default;

    ocmd_assembled_code assemble();

   private:
    struct unresolved_data_section {
      struct data_object {
        std::string name = "";
        data_type type = OCMD_DATA_TYPE_INVALID;
        uint16_t section_offset = 0;
        uint16_t size_in_bytes = 0;
      };

      std::string name = "";
      natural_t output_offset = 0;

      std::vector<data_object> objects = {};
    };

    struct unresolved_code_section {
      struct unresolved_instruction {
        uint32_t category_and_type = 0;
        uint16_t section_offset = 0;

        uint32_t opcode = 0;
        std::vector<raw_instruction::argument> arguments = {};
      };

      struct unresolved_jump_label {
        std::string name = "";
        uint16_t section_offset = 0;
      };

      std::string name = "";
      natural_t output_offset = 0;
      natural_t size_in_bytes = 0;

      std::vector<unresolved_instruction> instructions = {};
    };

    ocmd_ir ir;

    /// assembled code lacks address resolution
    std::vector<uint8_t> assembled_data = {};
    std::vector<uint8_t> assembled_code = {};

    natural_t num_instructions = 0;

    std::vector<ocmd_assembled_code::section_bound_ptr> code_section_bounds = {};
    std::vector<ocmd_assembled_code::unresolved_label> label_usages = {};
    std::vector<ocmd_assembled_code::data_object_ptr> data_object_ptrs = {};
    std::vector<unresolved_code_section> unresolved_code_sections = {};

    std::vector<unresolved_data_section> unresolved_data_sections = {};

    void assemble_code_sections();
    void assemble_data_sections();
    void resolve_local_labels();
  };

}  // namespace other

#endif  // OTHERLIB_VM_COMMAND_FILES_ASSEMBLER_HPP