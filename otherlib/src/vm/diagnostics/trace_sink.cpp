/**
 * \file vm/diagnostics/trace_sink.cpp
 **/
#include "vm/diagnostics/trace_sink.hpp"

#include "core/enum_formatter.hpp"
#include "core/logger.hpp"

namespace other {

  void trace_sink::handle(const diagnostic& diag) {
    // clang-format off
    CORE_LOG_DEBUG("[OCMD : {}] {}. Code:\n{}\nSuggested fix:\n{}", 
                    diag.severity, diag.final_message, diag.error_code, diag.suggested_fix.value_or("None"));
    // clang-format on
  }

}  // namespace other