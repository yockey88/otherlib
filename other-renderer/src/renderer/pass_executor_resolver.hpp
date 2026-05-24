/**
 * \file renderer/pass_executor_resolver.hpp
 **/
#ifndef OTHER_RENDERER_RENDERER_PASS_EXECUTOR_RESOLVER_HPP
#define OTHER_RENDERER_RENDERER_PASS_EXECUTOR_RESOLVER_HPP

#include "core/interfaces.hpp"

#include "gpu_resource/renderer_resource.hpp"
#include "renderer/pipeline_definition.hpp"
#include "renderer/render_pipeline.hpp"
#include "renderer/resource_tag.hpp"
#include "renderer/resource_tag_registry.hpp"

namespace other {

  class pass_executor_resolver {
    OTHER_ENVIRONMENT_INTERFACE("Renderer", "PassResolver");

   public:
    pass_executor_resolver() = default;
    virtual ~pass_executor_resolver() = default;

    virtual render_graph::pass_executor resolve_executor(std::string_view target, const pipeline_pass_definition& def, render_pipeline* pl) = 0;
    virtual resource_tag_binder_fn_t resolve_binder(std::string_view target, resource_tag tag) = 0;

   private:
  };

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_PASS_EXECUTOR_RESOLVER_HPP