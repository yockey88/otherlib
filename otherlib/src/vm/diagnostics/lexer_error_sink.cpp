/**
 * \file vm/diagnostics/lexer_error_sink.cpp
 **/
#include "vm/diagnostics/lexer_error_sink.hpp"

namespace other {

  void lexer_error_sink::handle(diagnostic& diag) {
    if (diag.severity >= VM_DIAGNOSTIC_WARNING) {
      const std::string formatted = default_format(diag);
      if (diag.severity == VM_DIAGNOSTIC_WARNING) {
        CORE_LOG_WARN("[LEXER WARNING] {}", formatted);
      } else if (diag.severity == VM_DIAGNOSTIC_ERROR) {
        CORE_LOG_ERROR("[LEXER ERROR] {}", formatted);
      }
    }
  }

}  // namespace other