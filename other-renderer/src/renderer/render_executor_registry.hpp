/**
 * \file renderer/render_executor_registry.hpp
 **/
#ifndef OTHER_RENDERER_RENDERER_RENDER_EXECUTOR_REGISTRY_HPP
#define OTHER_RENDERER_RENDERER_RENDER_EXECUTOR_REGISTRY_HPP

#include "renderer/render_graph.hpp"

namespace other {

  struct pipeline_pass_definition;
  class render_pipeline;

  using executor_factory_fn = std::function<render_graph::pass_executor(const pipeline_pass_definition&, render_pipeline*)>;

  class render_executor_registry {
   public:
    natural_t register_executor(std::string_view name, executor_factory_fn fn);
    executor_factory_fn find(std::string_view name) const;
    ostd::vector<std::string> known_executors() const;

   private:
    ostd::map<natural_t, std::string> debug_names;
    ostd::map<natural_t, executor_factory_fn> factories;
  };

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_RENDER_EXECUTOR_REGISTRY_HPP