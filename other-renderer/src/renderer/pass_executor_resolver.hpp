/**
 * \file renderer/pass_executor_resolver.hpp
 **/
#ifndef OTHER_RENDERER_RENDERER_PASS_EXECUTOR_RESOLVER_HPP
#define OTHER_RENDERER_RENDERER_PASS_EXECUTOR_RESOLVER_HPP

#include "core/interfaces.hpp"

#include "gpu_resource/renderer_resource.hpp"
#include "renderer/frame_binding_registry.hpp"
#include "renderer/pipeline_definition.hpp"
#include "renderer/render_pipeline.hpp"
#include "renderer/resource_tag.hpp"

namespace other {

  class pass_executor_resolver {
    OTHER_ENVIRONMENT_INTERFACE("Renderer", "PassResolver");

   public:
    pass_executor_resolver() = default;
    virtual ~pass_executor_resolver() = default;

    virtual bool can_resolve_executor(std::string_view name) const = 0;
    virtual bool can_resolve_frame_binder(resource_tag tag) const = 0;
    virtual bool can_resolve_draw_binder(resource_tag tag) const = 0;
    virtual bool can_resolve_instance_binder(resource_tag tag) const = 0;

    virtual render_graph::pass_executor resolve_executor(std::string_view target, const pipeline_pass_definition& def, render_pipeline* pl) = 0;
    virtual per_frame_producer_fn resolve_frame_binder(std::string_view target, resource_tag tag) = 0;
    virtual per_draw_producer_fn resolve_draw_binder(std::string_view target, resource_tag tag) = 0;
    virtual per_instance_producer_fn resolve_instance_binder(std::string_view target, resource_tag tag) = 0;

   private:
  };

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_PASS_EXECUTOR_RESOLVER_HPP