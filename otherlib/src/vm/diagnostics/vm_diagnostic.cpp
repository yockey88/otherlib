/**
 * \file vm/diagnostics/vm_diagnostic.cpp
 **/
#include "vm/diagnostics/vm_diagnostic.hpp"

#include "core/logger.hpp"

namespace other {

  diagnostic get_diagnostic(vm_error_code code) {
    OTHER_ASSERT(code < NUM_VM_ERROR_CODES, "Invalid Error Code: {:#04x}", static_cast<uint16_t>(code));
    return kDiagnostics[static_cast<size_t>(code)];
  }

}  // namespace other