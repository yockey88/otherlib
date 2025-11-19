/**
 * \file vm/command_files/linker.cpp
 **/
#include "vm/command_files/linker.hpp"

#include <algorithm>
#include <cstdint>

#include "vm/command_files/ocmd_headers.hpp"
#include "vm/other_device.hpp"

namespace other {

  std::vector<uint8_t> ocmd_linker::link() {
    ocmd_file_header file_header = {};
    auto& prog_header = file_header.prog_header;
    prog_header.num_instructions = static_cast<uint16_t>(assembled_codes.num_instructions);
    prog_header.code_section_offset = sizeof(ocmd_file_header);
    prog_header.data_section_offset = prog_header.code_section_offset + static_cast<uint16_t>(assembled_codes.code.size());
    prog_header.data_table_offset = prog_header.data_section_offset + static_cast<uint16_t>(assembled_codes.data.size());

    for (const auto& unresolved_lbl : assembled_codes.unresolved_labels) {
      auto& section = assembled_codes.code;

      uint8_t* instr_ptr = section.data() + unresolved_lbl.address;
      instruction* instr = reinterpret_cast<instruction*>(instr_ptr);

      auto itr = std::ranges::find_if(assembled_codes.code_section_bounds, [&](const ocmd_assembled_code::section_bound_ptr& sec) {
        return sec.name == unresolved_lbl.label_name;
      });
      if (itr != assembled_codes.code_section_bounds.end()) {
        uint16_t label_address = itr->offset;
        instr->lower = label_address + prog_header.code_section_offset;
      }
      /// otherwise could be a data object
      else {
        /// get data section name
        auto dot_pos = unresolved_lbl.label_name.find('.');
        if (dot_pos == std::string::npos) {
          continue;
        }

        std::string data_section_name = unresolved_lbl.label_name.substr(0, dot_pos);
        std::string data_object_name = unresolved_lbl.label_name.substr(dot_pos + 1);
        auto data_section_itr = std::ranges::find_if(assembled_codes.data_object_ptrs, [&](const ocmd_assembled_code::data_object_ptr& data_obj) {
          return data_obj.name == data_object_name;
        });
        if (data_section_itr != assembled_codes.data_object_ptrs.end()) {
          uint16_t data_object_address = static_cast<uint16_t>(data_section_itr->offset);
          instr->lower = data_object_address + prog_header.data_section_offset;
        }
      }
    }

    std::vector<uint8_t> data_object_table = {};

    uint16_t num_objects = static_cast<uint16_t>(assembled_codes.data_object_ptrs.size());
    const uint8_t* num_objects_bytes = reinterpret_cast<const uint8_t*>(&num_objects);
    data_object_table.append_range(std::span(num_objects_bytes, sizeof(uint16_t)));

    uint16_t obj_idx = 0;
    for (const auto& data_obj_ptr : assembled_codes.data_object_ptrs) {
      const uint8_t* idx_bytes = reinterpret_cast<const uint8_t*>(&obj_idx);
      data_object_table.append_range(std::span(idx_bytes, sizeof(uint16_t)));
      ++obj_idx;

      uint16_t offset = static_cast<uint16_t>(data_obj_ptr.offset + prog_header.data_section_offset);
      const uint8_t* offset_bytes = reinterpret_cast<const uint8_t*>(&offset);
      data_object_table.append_range(std::span(offset_bytes, sizeof(uint16_t)));

      uint16_t name_length = static_cast<uint16_t>(data_obj_ptr.name.size());
      const uint8_t* name_length_bytes = reinterpret_cast<const uint8_t*>(&name_length);
      data_object_table.append_range(std::span(name_length_bytes, sizeof(uint16_t)));

      const uint8_t* name_bytes = reinterpret_cast<const uint8_t*>(data_obj_ptr.name.c_str());
      data_object_table.append_range(std::span(name_bytes, name_length));
    }

    std::vector<uint8_t> final_binary = {};

    const uint8_t* header_bytes = reinterpret_cast<const uint8_t*>(&file_header);
    final_binary.append_range(std::span(header_bytes, sizeof(ocmd_file_header)));
    final_binary.append_range(assembled_codes.code);
    final_binary.append_range(assembled_codes.data);
    final_binary.append_range(data_object_table);
    if (final_binary.size() % other_command_device::kOpCodeSize != 0) {
      size_t padding_needed = other_command_device::kOpCodeSize - (final_binary.size() % other_command_device::kOpCodeSize);
      final_binary.insert(final_binary.end(), padding_needed, 0x00);
    }

    return final_binary;
  }

}  // namespace other