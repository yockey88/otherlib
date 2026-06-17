/**
 * \file ocmd_code.hpp
 **/
#ifndef OTHERLIB_VM_COMMAND_FILES_OCMD_CODE_HPP
#define OTHERLIB_VM_COMMAND_FILES_OCMD_CODE_HPP

#include "vm/command_files/token.hpp"

namespace other {

  struct ocmd_assembled_code {
    struct section_bound_ptr {
      std::string name = "";
      natural_t offset = 0;
      natural_t size = 0;
    };
    struct unresolved_label {
      uint16_t address = 0;
      std::string label_name = "";
    };
    struct data_object_ptr {
      std::string name = "";
      natural_t offset = 0;
      natural_t size = 0;
    };
    struct definition {
      std::string name;
      token value = { TOKEN_TYPE_INVALID, "", {} };
    };

    natural_t num_instructions = 0;

    std::vector<uint8_t> code = {};
    std::vector<uint8_t> data = {};

    std::vector<definition> definitions = {};
    std::vector<section_bound_ptr> code_section_bounds = {};
    std::vector<unresolved_label> unresolved_labels = {};
    std::vector<data_object_ptr> data_object_ptrs = {};

    bool malformed = true;
  };

}  // namespace other

#endif  // OTHERLIB_VM_COMMAND_FILES_OCMD_CODE_HPP