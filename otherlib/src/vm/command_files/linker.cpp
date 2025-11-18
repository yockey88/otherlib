/**
 * \file vm/command_files/linker.cpp
 **/
#include "vm/command_files/linker.hpp"

#include <algorithm>
#include <cstdint>

namespace other {

  std::vector<uint8_t> ocmd_linker::link() {
    std::vector<uint8_t> final_binary = {};
    final_binary.append_range(assembled_codes.code);
    final_binary.append_range(assembled_codes.data);

    for (const auto& unresolved_lbl : assembled_codes.unresolved_labels) {
      auto& section = assembled_codes.code;

      uint8_t* instr_ptr = section.data() + unresolved_lbl.address;
      instruction* instr = reinterpret_cast<instruction*>(instr_ptr);

      auto itr = std::ranges::find_if(assembled_codes.code_section_bounds, [&](const ocmd_assembled_code::section_bound_ptr& sec) {
        return sec.name == unresolved_lbl.label_name;
      });
      if (itr != assembled_codes.code_section_bounds.end()) {
        uint16_t label_address = itr->offset;
        instr->lower = label_address;
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
          instr->lower = data_object_address;
        }
      }
    }

    return final_binary;
  }

}  // namespace other