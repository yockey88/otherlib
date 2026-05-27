/**
 * \file vm/diagnostics/vm_diagnostic.hpp
 **/
#ifndef OTHERLIB_VM_DIAGNOSTICS_VM_DIAGNOSTIC_HPP
#define OTHERLIB_VM_DIAGNOSTICS_VM_DIAGNOSTIC_HPP

namespace other {

  struct source_location {
    std::string source_name;
    uint32_t line;
    uint32_t column;
    uint32_t byte_offset;
  };

  struct source_span {
    source_location start;
    source_location end;
  };

  enum diagnostic_severity : uint8_t {
    VM_DIAGNOSTIC_NOTE,
    VM_DIAGNOSTIC_WARNING,
    VM_DIAGNOSTIC_ERROR,
    VM_DIAGNOSTIC_FATAL
  };

  struct diagnostic_note {
    source_span span;
    std::string message;
  };

  struct diagnostic {
    diagnostic_severity severity = VM_DIAGNOSTIC_ERROR;
    source_span span;
    std::string message;
    std::string code = "";
    std::vector<diagnostic_note> notes = {};
    std::optional<std::string> suggested_fix;
  };

}  // namespace other

#endif  // OTHERLIB_VM_DIAGNOSTICS_VM_DIAGNOSTIC_HPP