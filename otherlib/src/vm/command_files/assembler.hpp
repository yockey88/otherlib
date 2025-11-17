/**
 * \file vm/command_files/assembler.hpp
 **/
#ifndef OTHERLIB_VM_COMMAND_FILES_ASSEMBLER_HPP
#define OTHERLIB_VM_COMMAND_FILES_ASSEMBLER_HPP

#include "vm/command_files/parser.hpp"

namespace other {

  class ocmd_assembler {
   public:
    ocmd_assembler(const ocmd_ir& ir)
        : ir(ir) {}
    ~ocmd_assembler() = default;

    std::vector<uint8_t> assemble();
    std::vector<uint8_t> basic_link();
    std::vector<uint8_t> assemble_and_basic_link();

   private:
    struct unresolved_data_section {
      struct data_object {
        natural_t name_hash = 0;
        data_type type = OCMD_DATA_TYPE_INVALID;
        uint16_t section_offset = 0;
        uint16_t size_in_bytes = 0;
      };

      std::string name = "";
      natural_t name_hash = 0;

      std::vector<data_object> objects = {};
      std::vector<uint8_t> data = {};
    };

    struct unresolved_code_section {
      struct unresolved_instruction {
        uint32_t category_and_type = 0;
        uint16_t section_offset = 0;

        uint32_t opcode = 0;
        std::vector<token> arguments = {};
      };

      std::string name = "";
      natural_t name_hash = 0;

      std::vector<unresolved_instruction> instructions = {};
      std::vector<uint8_t> data = {};
    };

    ocmd_ir ir;
    std::vector<uint8_t> assembled_code = {};
    std::vector<uint8_t> linked_code = {};

    std::vector<unresolved_data_section> unresolved_data_sections = {};
    std::vector<unresolved_code_section> unresolved_code_sections = {};

    void assemble_code_sections();
    void assemble_data_sections();
  };

}  // namespace other

#endif  // OTHERLIB_VM_COMMAND_FILES_ASSEMBLER_HPP