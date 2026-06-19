/**
 * \file vm/diagnostics/diagnostic_sink.cpp
 **/
#include "vm/diagnostics/diagnostic_sink.hpp"

#include "core/enum_formatter.hpp"

namespace other {

  std::string diagnostic_sink::default_format(diagnostic& d) const {
    // source_span span;
    // std::string final_message;

    // // notes/fix to format
    // std::vector<diagnostic_note> notes = {};
    // opt<std::string> suggested_fix = {};

    std::stringstream ss;
    if (!d.final_message.empty()) {
      ss << d.final_message << "\n";
    }
    for (const auto& note : d.notes) {
      ss << std::format("  - {} ([{}, {}])\n", note.message, note.span.start, note.span.end);
    }
    d.final_message = ss.str();

    return std::format("[OCMD : {}, {}, {}]:\n{}([{}, {}])",
                       d.severity, d.phase, d.error_code, d.final_message.empty() ? "<no message>" : d.final_message, d.span.start, d.span.end);
  }

}  // namespace other