/**
 * \file vm/diagnostics/vm_diagnostic.hpp
 **/
#ifndef OTHERLIB_VM_DIAGNOSTICS_VM_DIAGNOSTIC_HPP
#define OTHERLIB_VM_DIAGNOSTICS_VM_DIAGNOSTIC_HPP

#include "vm/diagnostics/error_codes.hpp"

namespace other {

  struct source_location {
    size_t line;
    size_t column;
    size_t byte_offset;
  };

  struct source_span {
    source_location start;
    source_location end;
  };

  struct source_view {
    source_span span;
    std::string_view code;

    auto begin() { return code.begin() + span.start.byte_offset; }
    auto begin() const { return code.begin() + span.start.byte_offset; }
    auto end() { return code.begin() + span.end.byte_offset; }
    auto end() const { return code.begin() + span.end.byte_offset; }

    auto size() const { return span.end.byte_offset - span.start.byte_offset; }
    auto empty() const { return size() == 0; }
  };

  enum diagnostic_severity : uint8_t {
    VM_DIAGNOSTIC_TRACE,
    VM_DIAGNOSTIC_INFO,
    VM_DIAGNOSTIC_WARNING,
    VM_DIAGNOSTIC_ERROR,
    VM_DIAGNOSTIC_FATAL
  };

  struct diagnostic_note {
    source_span span;
    std::string message;
  };

  enum vm_phase : uint8_t {
    VM_PHASE_LEXER = 0x01,
    VM_PHASE_PARSER = 0x02,
    VM_PHASE_SEMA = 0x03,
    VM_PHASE_COMPILER = 0x04,
    VM_PHASE_LINKER = 0x05,
    VM_PHASE_RUNTIME = 0x06,

    NUM_VM_PHASES = VM_PHASE_RUNTIME,
    INVALID_VM_PHASE = 0x00
  };

  struct diagnostic {
    diagnostic_severity severity = VM_DIAGNOSTIC_ERROR;
    vm_error_code error_code;
    vm_phase phase;

    /// formatted/final
    source_span span;
    std::string final_message;

    // notes/fix to format
    std::vector<diagnostic_note> notes = {};
    opt<std::string> suggested_fix = {};
  };

  const inline std::array kDiagnostics{
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

  diagnostic get_diagnostic(vm_error_code code);

}  // namespace other

#endif  // OTHERLIB_VM_DIAGNOSTICS_VM_DIAGNOSTIC_HPP