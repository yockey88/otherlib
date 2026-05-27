/**
 * \file vm/diagnostics/diagnostic_sink.hpp
 **/
#ifndef OTHERLIB_VM_DIAGNOSTICS_DIAGNOSTIC_SINK_HPP
#define OTHERLIB_VM_DIAGNOSTICS_DIAGNOSTIC_SINK_HPP

#include "core/interfaces.hpp"

#include "vm/diagnostics/error_codes.hpp"
#include "vm/diagnostics/vm_diagnostic.hpp"

namespace other {

  class diagnostic_sink {
    OTHER_ENVIRONMENT_INTERFACE("VM", "DiagnosticSink");

   public:
    virtual ~diagnostic_sink() = default;

    virtual void emit(const diagnostic& diag) = 0;
  };

}  // namespace other

#endif  // OTHERLIB_VM_DIAGNOSTICS_DIAGNOSTIC_SINK_HPP