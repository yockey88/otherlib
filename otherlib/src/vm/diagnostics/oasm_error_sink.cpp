/**
 * \file vm/diagnostics/oasm_error_sink.cpp
 **/
#include "vm/diagnostics/oasm_error_sink.hpp"

namespace other {

  void oasm_error_sink::handle(const diagnostic& diag) {
    if (diag.severity >= VM_DIAGNOSTIC_WARNING) {
      const std::string formatted = default_format(diag);
      if (diag.severity == VM_DIAGNOSTIC_WARNING) {
        CORE_LOG_WARN("[OASM WARNING] {}", formatted);
      } else if (diag.severity == VM_DIAGNOSTIC_ERROR) {
        CORE_LOG_ERROR("[OASM ERROR] {}", formatted);
      }
    }
  }

}  // namespace other