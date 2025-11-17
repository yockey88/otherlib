/**
 * \file vm/command_files/assembler.cpp
 **/
#include "vm/command_files/assembler.hpp"

#include "core/fnv.hpp"

#include "token.hpp"

namespace other {

  std::vector<uint8_t> ocmd_assembler::assemble() {
    assemble_data_sections();
    assemble_code_sections();
    return assembled_code;
  }

  std::vector<uint8_t> ocmd_assembler::basic_link() {
    /// do linking
    return linked_code;
  }

  std::vector<uint8_t> ocmd_assembler::assemble_and_basic_link() {
    assemble();
    basic_link();
    return linked_code;
  }

  void ocmd_assembler::assemble_code_sections() {
    for (const auto& [name_hash, code_section] : ir.code_blocks) {
      unresolved_code_section& unresolved_section = unresolved_code_sections.emplace_back();
      unresolved_section.name = code_section.name;
      unresolved_section.name_hash = name_hash;

      for (const auto& instr : code_section.instructions) {
        unresolved_code_section::unresolved_instruction& unresolved_instr = unresolved_section.instructions.emplace_back();
        unresolved_instr.category_and_type = instr.category_and_type;

        uint32_t opcode = opcode_get_from_category_and_type(instr.category_and_type);

        for (const auto& arg : instr.arguments) {
          opcode = raw_instruction::get_opcode(instr.category_and_type, instr.arguments);
        }

        unresolved_instr.opcode = opcode;
      };
    }

    /// write all code to assembled_code keeping sections aligned
    for (const auto& section : unresolved_code_sections) {
      // /// align to 16 bytes
      // while (assembled_code.size() % 16 != 0) {
      //   assembled_code.push_back(0x00);
      // }

      for (const auto& instr : section.instructions) {
        assembled_code.push_back(static_cast<uint8_t>((instr.opcode >> 24) & 0xFF));
        assembled_code.push_back(static_cast<uint8_t>((instr.opcode >> 16) & 0xFF));
        assembled_code.push_back(static_cast<uint8_t>((instr.opcode >> 8) & 0xFF));
        assembled_code.push_back(static_cast<uint8_t>(instr.opcode & 0xFF));
      }
    }
  }

  void ocmd_assembler::assemble_data_sections() {
    for (const auto& [name_hash, data_section] : ir.data_blocks) {
      unresolved_data_section& unresolved_section = unresolved_data_sections.emplace_back();
      unresolved_section.name = data_section.name;
      unresolved_section.name_hash = name_hash;

      for (const auto& obj : data_section.objects) {
        unresolved_data_section::data_object& unresolved_obj = unresolved_section.objects.emplace_back();
        unresolved_obj.name_hash = FNV(obj.name);
        unresolved_obj.type = obj.type;
        unresolved_obj.section_offset = static_cast<uint16_t>(unresolved_section.data.size());
        unresolved_obj.size_in_bytes = static_cast<uint16_t>(obj.data.size());
        std::ranges::copy(obj.data, std::back_inserter(unresolved_section.data));
      };
    }
  }

}  // namespace other