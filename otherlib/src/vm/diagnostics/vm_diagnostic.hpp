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
    ostd::vector<diagnostic_note> notes = {};
    opt<std::string> suggested_fix = {};
  };

  diagnostic get_diagnostic(vm_error_code code);

}  // namespace other

namespace std {

  template <>
  struct formatter<other::source_location> : public std::formatter<std::string> {
    template <typename FormatContext>
    auto format(const other::source_location& loc, FormatContext& ctx) const {
      constexpr std::string_view fmt_str = "{}:{} (@ {} bytes)";
      return std::format_to(ctx.out(), fmt_str,
                            loc.line == static_cast<size_t>(-1) ? 0 : loc.line,
                            loc.column == static_cast<size_t>(-1) ? 0 : loc.column,
                            loc.byte_offset == static_cast<size_t>(-1) ? 0 : loc.byte_offset);
    }
  };

}  // namespace std

#endif  // OTHERLIB_VM_DIAGNOSTICS_VM_DIAGNOSTIC_HPP