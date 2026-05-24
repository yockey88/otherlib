/**
 * \file renderer/default_pass_executor_resolver.hpp
 **/
#ifndef OTHERLIB_RENDERER_DEFAULT_PASS_EXECUTOR_RESOLVER_HPP
#define OTHERLIB_RENDERER_DEFAULT_PASS_EXECUTOR_RESOLVER_HPP

#include "renderer/pass_executor_resolver.hpp"

namespace other {

  class default_pass_executor_resolver : public pass_executor_resolver {
   public:
    default_pass_executor_resolver(bool enable_scripted_executors)
        : scripts_enabled(enable_scripted_executors) {}
    ~default_pass_executor_resolver() override {}
    render_graph::pass_executor resolve_executor(std::string_view target, const pipeline_pass_definition& def, render_pipeline* pl) override;
    resource_tag_binder_fn_t resolve_binder(std::string_view target, resource_tag tag) override;

   private:
    bool scripts_enabled = true;

    std::string get_dotnet_class_name(const std::string_view entry);
    std::string get_dotnet_method_name(const std::string_view entry);

    render_graph::pass_executor dotnet_make_pass_executor(const std::string_view entry, const pipeline_pass_definition def, render_pipeline* pl);
    render_graph::pass_executor lua_make_pass_executor(const std::string_view entry, const pipeline_pass_definition def, render_pipeline* pl);
    render_graph::pass_executor vm_make_pass_executor(const std::string_view entry, const pipeline_pass_definition def, render_pipeline* pl);

    resource_tag_binder_fn_t dotnet_make_resource_tag_binder(const std::string_view entry, resource_tag tag);
    resource_tag_binder_fn_t lua_make_resource_tag_binder(const std::string_view entry, resource_tag tag);
    resource_tag_binder_fn_t vm_make_resource_tag_binder(const std::string_view entry, resource_tag tag);
  };

}  // namespace other

#endif  // OTHERLIB_RENDERER_DEFAULT_PASS_EXECUTOR_RESOLVER_HPP