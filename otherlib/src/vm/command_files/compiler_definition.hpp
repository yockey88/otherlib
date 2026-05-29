/**
 * \file vm/command_files/compiler_definition.hpp
 **/
#ifndef OTHERLIB_VM_COMMAND_FILES_COMPILER_DEFINITION_HPP
#define OTHERLIB_VM_COMMAND_FILES_COMPILER_DEFINITION_HPP

#include "vm/command_files/token.hpp"

namespace other {

  struct compiler_definition {
    std::string name;
    token value = { TOKEN_TYPE_INVALID, "", source_span{ { 0, 0 }, { 0, 0 } } };
  };

}  // namespace other

#endif  // OTHERLIB_VM_COMMAND_FILES_COMPILER_DEFINITION_HPP