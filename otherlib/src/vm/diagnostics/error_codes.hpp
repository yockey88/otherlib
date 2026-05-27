/**
 * \file vm/diagnostics/error_codes.hpp
 **/
#ifndef OTHERLIB_VM_DIAGNOSTICS_ERROR_CODES_HPP
#define OTHERLIB_VM_DIAGNOSTICS_ERROR_CODES_HPP

#include <array>
#include <string>

namespace other {

  enum vm_error_code : uint32_t {
    VM_UNKNOWN_ERROR = 0x0000,

    VM_LEXER_ERROR_UNKNOWN,
    VM_PARSER_ERROR_UNKNOWN,
    VM_COMPILER_ERROR_UNKNOWN,
    VM_LINKER_ERROR_UNKNOWN,

    NUM_VM_ERROR_CODES,
    INVALID_VM_ERROR_CODE = NUM_VM_ERROR_CODES,
  };

  struct vm_error_code_description {
    vm_error_code code;
    const std::string_view message;
    constexpr vm_error_code_description(vm_error_code code, const std::string_view message)
        : code(code), message(message) {}
  };

  constexpr inline size_t kNumErrorCodes = NUM_VM_ERROR_CODES;
  constexpr inline std::array<vm_error_code_description, kNumErrorCodes> kErrorCodeDescriptions = {
    vm_error_code_description{ VM_UNKNOWN_ERROR, "An unknown error occurred" },
    vm_error_code_description{ VM_LEXER_ERROR_UNKNOWN, "An unknown lexer error occurred" },
    vm_error_code_description{ VM_PARSER_ERROR_UNKNOWN, "An unknown parser error occurred" },
    vm_error_code_description{ VM_COMPILER_ERROR_UNKNOWN, "An unknown compiler error occurred" },
    vm_error_code_description{ VM_LINKER_ERROR_UNKNOWN, "An unknown linker error occurred" },
  };

}  // namespace other

#endif  // OTHERLIB_VM_DIAGNOSTICS_ERROR_CODES_HPP