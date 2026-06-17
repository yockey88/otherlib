/**
 * \file vm/diagnostics/lexer_error_sink.hpp
 **/
#ifndef OTHERLIB_VM_DIAGNOSTICS_LEXER_ERROR_SINK_HPP
#define OTHERLIB_VM_DIAGNOSTICS_LEXER_ERROR_SINK_HPP

#include "vm/diagnostics/diagnostic_sink.hpp"

namespace other {

  class lexer_error_sink : public diagnostic_sink {
   public:
    ~lexer_error_sink() override = default;

    void handle(diagnostic& diag) override;
  };

}  // namespace other

#endif  // OTHERLIB_VM_DIAGNOSTICS_LEXER_ERROR_SINK_HPP
