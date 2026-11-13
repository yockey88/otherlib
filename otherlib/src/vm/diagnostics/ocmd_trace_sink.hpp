/**
 * \file vm/diagnostics/ocmd_trace_sink.hpp
 **/
#ifndef OTHERLIB_VM_DIAGNOSTICS_OCMD_TRACE_SINK_HPP
#define OTHERLIB_VM_DIAGNOSTICS_OCMD_TRACE_SINK_HPP

#include "vm/diagnostics/diagnostic_sink.hpp"

namespace other {

  class ocmd_trace_sink : public diagnostic_sink {
   public:
    virtual ~ocmd_trace_sink() = default;

    void handle(diagnostic& diag) override;
  };

}  // namespace other

#endif  // OTHERLIB_VM_DIAGNOSTICS_OCMD_TRACE_SINK_HPP