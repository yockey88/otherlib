/**
 * \file vm/diagnostics/vm_diagnostic.cpp
 **/
#include "vm/diagnostics/vm_diagnostic.hpp"

#include "core/logger.hpp"

namespace other {

  diagnostic get_diagnostic(vm_error_code code) {
    OTHER_ASSERT(code < NUM_VM_ERROR_CODES, "Invalid Error Code: {:#04x}", static_cast<uint16_t>(code));
    static const std::array kDiagnostics{
      diagnostic{ VM_DIAGNOSTIC_FATAL, INVALID_VM_ERROR_CODE, INVALID_VM_PHASE, {}, "COMPILER BUG. INVALID ERROR CODE" },

      diagnostic{ VM_DIAGNOSTIC_ERROR, LEX_INVALID_CHAR, VM_PHASE_LEXER, {}, "unexpected character '{}'" },
      diagnostic{ VM_DIAGNOSTIC_ERROR, LEX_UNTERMINATED_STRING, VM_PHASE_LEXER, {}, "unterminated string literal" },
      diagnostic{ VM_DIAGNOSTIC_ERROR, LEX_MULTIPLE_DECIMAL, VM_PHASE_LEXER, {}, "multiple decimal points in number" },
      diagnostic{ VM_DIAGNOSTIC_ERROR, LEX_NUMBER_OUT_OF_RANGE, VM_PHASE_LEXER, {}, "number out of range" },
      diagnostic{ VM_DIAGNOSTIC_ERROR, LEX_UNTERMINATED_COMMENT, VM_PHASE_LEXER, {}, "unterminated comment" },

      diagnostic{ VM_DIAGNOSTIC_ERROR, PARSE_UNEXPECTED_TOKEN, VM_PHASE_PARSER, {}, "unexpected {}, expected {}" },
      diagnostic{ VM_DIAGNOSTIC_ERROR, PARSE_EXPECTED_TOKEN, VM_PHASE_PARSER, {}, "expected '{}'" },
      diagnostic{ VM_DIAGNOSTIC_ERROR, PARSE_UNKNOWN_DIRECTIVE, VM_PHASE_PARSER, {}, "unknown directive '{}'" },
      diagnostic{ VM_DIAGNOSTIC_ERROR, PARSE_TOO_MANY_OPERANDS, VM_PHASE_PARSER, {}, "'{}' takes at most {} operands, found {}" },
      diagnostic{ VM_DIAGNOSTIC_ERROR, PARSE_DUPLICATE_DATA_OBJ, VM_PHASE_PARSER, {}, "duplicate data object '{}' in block '{}'" },

      diagnostic{ VM_DIAGNOSTIC_ERROR, COMPILER_UNDEFINED_LABEL, VM_PHASE_COMPILER, {}, "reference to undefined label '{}'" },
      diagnostic{ VM_DIAGNOSTIC_ERROR, COMPILER_UNUSED_LABEL, VM_PHASE_COMPILER, {}, "label '{}' is never referenced" },

      diagnostic{ VM_DIAGNOSTIC_ERROR, LINK_UNRESOLVED_SYMBOL, VM_PHASE_LINKER, {}, "unresolved symbol '{}'" },
    };
    return kDiagnostics[static_cast<size_t>(code)];
  }

}  // namespace other