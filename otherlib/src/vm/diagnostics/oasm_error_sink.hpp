/**
 * \file diangostics/oasm_error_sink.hpp
 **/
#ifndef OTHERLIB_VM_DIAGNOSTICS_OASM_ERROR_SINK_HPP
#define OTHERLIB_VM_DIAGNOSTICS_OASM_ERROR_SINK_HPP

#include "vm/diagnostics/diagnostic_sink.hpp"

namespace other {

  class oasm_error_sink : public diagnostic_sink {
   public:
    ~oasm_error_sink() override = default;

    void handle(const diagnostic& diag) override;
  };

}  // namespace other

#endif  // OTHERLIB_VM_DIAGNOSTICS_OASM_ERROR_SINK_HPP