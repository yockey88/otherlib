/**
 * \file vm/diagnostics/ocmd_errors.hpp
 **/
#ifndef OTHERLIB_VM_DIAGNOSTICS_OCMD_ERRORS_HPP
#define OTHERLIB_VM_DIAGNOSTICS_OCMD_ERRORS_HPP

#include <exception>

#include "vm/diagnostics/vm_diagnostic.hpp"

namespace other {

  class lexer_error : public std::runtime_error {
   public:
    ~lexer_error() override {}
    lexer_error(vm_error_code error, source_span loc, const std::string_view msg)
        : std::runtime_error(std::string(msg)), error(error), loc(loc), msg(msg) {}

    vm_error_code error;
    source_span loc;
    std::string msg;
  };

}  // namespace other

#endif  // OTHERLIB_VM_DIAGNOSTICS_OCMD_ERRORS_HPP