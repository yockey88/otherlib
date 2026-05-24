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
    render_graph::pass_executor resolve_executor(std::string_view target, const pipeline_pass_definition& def, render_pipeline* pl) override;
    resource_tag_binder_fn_t resolve_binder(std::string_view target, resource_tag tag) override;

   private:
    scripting_system* system;

    render_graph::pass_executor create_callback_executor(const std::string_view entry, const pipeline_pass_definition def, render_pipeline* pl);
    resource_tag_binder_fn_t create_callback_tag_binder(const std::string_view entry, resource_tag tag);
  };

}  // namespace other

#endif  // OTHERLIB_RENDER_DEFAULT_PASS_EXECUTOR_RESOLVER_HPP