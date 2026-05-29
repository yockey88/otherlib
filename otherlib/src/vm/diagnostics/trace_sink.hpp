/**
 * \file vm/diagnostics/trace_sink.hpp
 **/
#ifndef OTHERLIB_SRC_VM_DIAGNOSTICS_TRACE_SINK_HPP
#define OTHERLIB_SRC_VM_DIAGNOSTICS_TRACE_SINK_HPP

#include "vm/diagnostics/diagnostic_sink.hpp"

namespace other {

  class trace_sink : public diagnostic_sink {
   public:
    virtual ~trace_sink() = default;

    void handle(const diagnostic& diag) override;
  };

}  // namespace other

#endif  // OTHERLIB_SRC_VM_DIAGNOSTICS_TRACE_SINK_HPP