/**
 * \file vm/diagnostics/ocmd_trace_sink.cpp
 **/
#include "vm/diagnostics/ocmd_trace_sink.hpp"

#include "core/enum_formatter.hpp"
#include "core/logger.hpp"

namespace other {

  void ocmd_trace_sink::handle(diagnostic& diag) {
    CORE_LOG_TRACE("{}", default_format(diag));
  }

}  // namespace other