/**
 * \file vm/command_files/assembler.cpp
 **/
#include "vm/command_files/assembler.hpp"

#include "core/logger.hpp"

#include "vm/control_table.hpp"
#include "vm/other_device.hpp"

#include "assembler.hpp"
#include "token.hpp"

namespace other {

  ocmd_assembled_code ocmd_assembler::assemble() {
    assemble_data_sections();
    assemble_code_sections();

    /// we can possibly resolve as many local labels here as possible
    resolve_local_labels();

    return {
      .num_instructions = num_instructions,
      .code = assembled_code,
      .data = assembled_data,
      .code_section_bounds = code_section_bounds,
      .unresolved_labels = label_usages,
      .data_object_ptrs = data_object_ptrs,
    };
  }

  void ocmd_assembler::assemble_code_sections() {
    natural_t section_offset = 0;
    for (const auto& [name_hash, code_section] : ir.code_blocks) {
      unresolved_code_section& unresolved_section = unresolved_code_sections.emplace_back();
      unresolved_section.name = code_section.name;
      unresolved_section.output_offset = section_offset;

      natural_t instr_offset = 0;
      for (natural_t instr_idx = 0; instr_idx < code_section.instructions.size(); ++instr_idx) {
        const auto& instr = code_section.instructions[instr_idx];

        unresolved_code_section::unresolved_instruction& unresolved_instr = unresolved_section.instructions.emplace_back();
        unresolved_instr.opcode = raw_instruction::get_opcode(instr.category_and_type, instr.arguments);
        unresolved_instr.arguments = instr.arguments;

        uint16_t current_offset = static_cast<uint16_t>(assembled_code.size());
        OTHER_ASSERT(current_offset == section_offset + instr_offset, "Assembled code offset mismatch");

        instruction i = unresolved_instr.opcode;
        if (i.lower == 0xFFFF) {
          for (natural_t arg_idx = 0; arg_idx < instr.arguments.size(); ++arg_idx) {
            if (instr.arguments[arg_idx].type != TOKEN_TYPE_LABEL) {
              continue;
            }

            ocmd_assembled_code::unresolved_label& unresolved_lbl = label_usages.emplace_back();
            unresolved_lbl.address = current_offset;
            unresolved_lbl.label_name = instr.arguments[arg_idx].raw_txt;
          }
        }

        unresolved_instr.section_offset = current_offset;
        instr_offset += other_command_device::kOpCodeSize;

        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&i.opcode);
        assembled_code.append_range(std::span(bytes, other_command_device::kOpCodeSize));
        ++num_instructions;
      }

      section_offset += unresolved_section.instructions.size() * other_command_device::kOpCodeSize;
    }

    for (const auto& unresolved_section : unresolved_code_sections) {
      code_section_bounds.emplace_back(ocmd_assembled_code::section_bound_ptr{
        .name = unresolved_section.name,
        .offset = unresolved_section.output_offset,
        .size = static_cast<natural_t>(unresolved_section.instructions.size() * other_command_device::kOpCodeSize),
      });
    }
  }

  void ocmd_assembler::assemble_data_sections() {
    uint16_t section_offset = 0;
    for (const auto& [name_hash, data_section] : ir.data_blocks) {
      unresolved_data_section& unresolved_section = unresolved_data_sections.emplace_back();
      unresolved_section.output_offset = section_offset;
      unresolved_section.name = data_section.name;

      natural_t data_offset = 0;
      natural_t start_offset = assembled_data.size();
      for (const auto& obj : data_section.objects) {
        unresolved_data_section::data_object& unresolved_obj = unresolved_section.objects.emplace_back();
        unresolved_obj.name = obj.name;
        unresolved_obj.type = obj.type;
        unresolved_obj.section_offset = data_offset;
        unresolved_obj.size_in_bytes = static_cast<uint16_t>(obj.data.size());

        data_object_ptrs.emplace_back(ocmd_assembled_code::data_object_ptr{
          .name = obj.name,
          .offset = section_offset + data_offset,
          .size = obj.data.size(),
        });

        assembled_data.append_range(obj.data);
        data_offset += unresolved_obj.size_in_bytes;
      }
      natural_t end_offset = assembled_data.size();
      natural_t total_size = end_offset - start_offset;
      section_offset += total_size;
    }
  }

  void ocmd_assembler::resolve_local_labels() {
    /// attempt to resolve as many labels as possible with just the infomation we have
  }

}  // namespace other