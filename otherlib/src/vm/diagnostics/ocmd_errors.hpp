/**
 * \file vm/diagnostics/ocmd_errors.hpp
 **/
#ifndef OTHERLIB_VM_DIAGNOSTICS_OCMD_ERRORS_HPP
#define OTHERLIB_VM_DIAGNOSTICS_OCMD_ERRORS_HPP

#include <format>
#include <stdexcept>

#include "core/enum_formatter.hpp"

#include "vm/diagnostics/vm_diagnostic.hpp"

namespace other {

  class ocmd_toolchain_error : public std::runtime_error {
   public:
    ~ocmd_toolchain_error() override {}
    ocmd_toolchain_error(vm_error_code error, source_span loc, const std::string_view msg)
        : std::runtime_error(std::string(msg)), error(error), loc(loc), msg(msg) {}
    ocmd_toolchain_error(vm_error_code error, const std::string_view msg)
        : std::runtime_error(std::string(msg)), error(error), loc(source_span{}), msg(msg) {}

    vm_error_code error;
    source_span loc;
    std::string msg;
  };

}  // namespace other

#endif  // OTHERLIB_VM_DIAGNOSTICS_OCMD_ERRORS_HPP