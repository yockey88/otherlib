/**
 * \file vm/diagnostics/diagnostic_engine.hpp
 **/
#ifndef OTHER_VM_DIAGNOSTICS_DIAGNOSTIC_ENGINE_HPP
#define OTHER_VM_DIAGNOSTICS_DIAGNOSTIC_ENGINE_HPP

#include "vm/diagnostics/source_map.hpp"
#include "vm/diagnostics/vm_diagnostic.hpp"

namespace other {

  class diagnostic_sink;

  class diagnostic_engine {
   public:
    diagnostic_engine() = default;
    ~diagnostic_engine() = default;

    natural_t register_sink(const std::string_view name, diagnostic_sink* sink);
    void remove_sink(const natural_t sink_id);

    void emit(const diagnostic& diag);
    void emit(const natural_t sink_id, const diagnostic& diag);

   private:
    source_map sources;
    std::map<natural_t, diagnostic_sink*> sinks;
  };

}  // namespace other

#endif  // OTHER_VM_DIAGNOSTICS_DIAGNOSTIC_ENGINE_HPP