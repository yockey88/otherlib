/**
 * \file vm/diagnostics/diagnostic_sink.cpp
 **/
#include "vm/diagnostics/diagnostic_sink.hpp"

#include "core/enum_formatter.hpp"

namespace other {

  std::string diagnostic_sink::default_format(const diagnostic& d) const {
    // source_span span;
    // std::string final_message;

    // // notes/fix to format
    // std::vector<diagnostic_note> notes = {};
    // opt<std::string> suggested_fix = {};

    return std::format("[OCMD : {}, {}, {}]: {} ([{}, {}])",
                       d.severity, d.phase, d.error_code, d.final_message, d.span.start, d.span.end);
  }

}  // namespace other