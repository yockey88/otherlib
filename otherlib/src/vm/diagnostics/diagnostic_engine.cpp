/**
 * \file vm/diagnostics/diagnostic_engine.cpp
 **/
#include "vm/diagnostics/diagnostic_engine.hpp"

#include "core/fnv.hpp"

#include "vm/diagnostics/diagnostic_sink.hpp"

namespace other {

  natural_t diagnostic_engine::register_sink(const std::string_view name, diagnostic_sink* sink) {
    OTHER_ASSERT(sink != nullptr, "diagnostic sink {} is null", name);

    natural_t hash = FNV(name);
    auto [itr, inserted] = sinks.emplace(hash, sink);
    if (!inserted) {
      CORE_LOG_ERROR("diagnostic sink {} is already registered", name);
      return 0;
    }
    itr->second->set_name(name);

    return hash;
  }

  void diagnostic_engine::remove_sink(const natural_t sink_id) {
    if (sinks.erase(sink_id) == 0) {
      CORE_LOG_ERROR("diagnostic sink {} is not registered", sink_id);
    }
  }

  void diagnostic_engine::emit(const diagnostic& diag) {
    for (auto& [sink_id, sink] : sinks) {
      OTHER_ASSERT(sink != nullptr, "diagnostic sink {} is null", sink_id);

      diagnostic copy = diag;
      sink->handle(copy);
    }
  }

  void diagnostic_engine::emit(const natural_t sink_id, const diagnostic& diag) {
    if (auto itr = sinks.find(sink_id); itr != sinks.end()) {
      OTHER_ASSERT(itr->second != nullptr, "diagnostic sink {} is null", sink_id);

      diagnostic copy = diag;
      itr->second->handle(copy);
    } else {
      CORE_LOG_ERROR("diagnostic sink {} is not registered", sink_id);
    }
  }

}  // namespace other