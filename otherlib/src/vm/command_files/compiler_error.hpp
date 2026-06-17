/**
 * \file vm/command_files/compiler_error.hpp
 **/
#ifndef OTHERLIB_VM_COMMAND_FILES_COMPILER_ERROR_HPP
#define OTHERLIB_VM_COMMAND_FILES_COMPILER_ERROR_HPP

#include <exception>

namespace other {

  class ocmd_lowering_error : public std::runtime_error {
   public:
    ocmd_lowering_error(const std::string& msg)
        : std::runtime_error(msg) {}
  };

  class ocmd_compilation_error : public std::runtime_error {
   public:
    ocmd_compilation_error(const std::string& msg)
        : std::runtime_error(msg) {}
  };

  class ocmd_symbol_resolution_error : public std::runtime_error {
   public:
    ocmd_symbol_resolution_error(const std::string& msg)
        : std::runtime_error(msg) {}
  };

  class ocmd_linking_error : public std::runtime_error {
   public:
    ocmd_linking_error(const std::string& msg)
        : std::runtime_error(msg) {}
  };

}  // namespace other

#endif  // OTHERLIB_VM_COMMAND_FILES_COMPILER_ERROR_HPP