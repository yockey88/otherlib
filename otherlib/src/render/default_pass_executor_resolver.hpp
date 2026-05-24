/**
 * \file render/default_pass_executor_resolver.hpp
 **/
#ifndef OTHERLIB_RENDER_DEFAULT_PASS_EXECUTOR_RESOLVER_HPP
#define OTHERLIB_RENDER_DEFAULT_PASS_EXECUTOR_RESOLVER_HPP

#include "renderer/pass_executor_resolver.hpp"

namespace other {

  class scripting_system;

  class default_pass_executor_resolver : public pass_executor_resolver {
   public:
    default_pass_executor_resolver(scripting_system* system)
        : system(system) {}
    ~default_pass_executor_resolver() override {}

    bool can_resolve_executor(std::string_view name) const override;
    bool can_resolve_frame_binder(resource_tag tag) const override;
    bool can_resolve_draw_binder(resource_tag tag) const override;
    bool can_resolve_instance_binder(resource_tag tag) const override;

    render_graph::pass_executor resolve_executor(std::string_view target, const pipeline_pass_definition& def, render_pipeline* pl) override;
    per_frame_producer_fn resolve_frame_binder(std::string_view target, resource_tag tag) override;
    per_draw_producer_fn resolve_draw_binder(std::string_view target, resource_tag tag) override;
    per_instance_producer_fn resolve_instance_binder(std::string_view target, resource_tag tag) override;

   private:
    scripting_system* system;

    render_graph::pass_executor create_callback_executor(const std::string_view entry, const pipeline_pass_definition def, render_pipeline* pl);
    per_frame_producer_fn create_callback_frame_binder(const std::string_view entry, resource_tag tag);
    per_draw_producer_fn create_callback_draw_binder(const std::string_view entry, resource_tag tag);
    per_instance_producer_fn create_callback_instance_binder(const std::string_view entry, resource_tag tag);
  };

}  // namespace other

#endif  // OTHERLIB_RENDER_DEFAULT_PASS_EXECUTOR_RESOLVER_HPP