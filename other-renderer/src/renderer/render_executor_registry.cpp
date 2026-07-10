/**
 * \file renderer/render_executor_registry.cpp
 **/
#include "renderer/render_executor_registry.hpp"

#include "core/fnv.hpp"

namespace other {

  natural_t render_executor_registry::register_executor(std::string_view name, executor_factory_fn fn) {
    OTHER_ASSERT(fn != nullptr, "Can not register null executor factory! {}", name);
    natural_t id = FNV(name);
    if (auto itr = debug_names.find(id); itr != debug_names.end()) {
      CORE_LOG_ERROR("Executor: {} alread registered (id: {})", name, id);
    } else {
      {
        auto [itr2, success] = debug_names.emplace(id, name);
        OTHER_ASSERT(success, "Failed to insert executor factory debug name! {} ({}))", name, id);
      }
      {
        auto [itr2, success] = factories.emplace(id, fn);
        OTHER_ASSERT(success, "Failed to insert executor factory! {} ({}))", name, id);
      }
    }

    return id;
  }

  executor_factory_fn render_executor_registry::find(std::string_view name) const {
    auto id = FNV(name);
    auto itr = factories.find(id);
    if (itr == factories.end()) {
      CORE_LOG_ERROR("Failed to find Executor: {} ({})", name, id);
      return nullptr;
    }
    return itr->second;
  }

  ostd::vector<std::string> render_executor_registry::known_executors() const {
    return debug_names | std::views::values | std::ranges::to<ostd::vector<std::string>>();
  }

}  // namespace other