/**
 * \file render/default_pass_executor_resolver.cpp
 **/
#include "render/default_pass_executor_resolver.hpp"

#include <format>
#include <string>

#include "renderer/pass_executor_interop.hpp"
#include "script/scripting_environment.hpp"

#include "driver/driver.hpp"
#include "driver/systems/scripting_system.hpp"

namespace other {

  render_graph::pass_executor default_pass_executor_resolver::resolve_executor(std::string_view target, const pipeline_pass_definition& def, render_pipeline* pl) {
    // target = "cs:Game.Pipelines.Foo", "lua:mod.fn", "vm:prog@label"
    auto colon = target.find(':');
    OTHER_ASSERT(colon != std::string_view::npos, "script target '{}' missing host prefix", target);
    std::string_view host = target.substr(0, colon);
    std::string_view entry = target.substr(colon + 1);

    if (host == "cs" || host == "lua" || host == "vm" || host == "plugin") {
      return create_callback_executor(entry, def, pl);
    }
    /// \todo handle other host types here

    return nullptr;
  }

  resource_tag_binder_fn_t default_pass_executor_resolver::resolve_binder(std::string_view target, resource_tag tag) {
    auto colon = target.find(':');
    OTHER_ASSERT(colon != std::string_view::npos, "script taget '{}' missing host prefix", target);
    std::string_view host = target.substr(0, colon);
    std::string_view entry = target.substr(colon + 1);

    if (host == "cs" || host == "lua" || host == "vm") {
      return create_callback_tag_binder(entry, tag);
    }

    return nullptr;
  }

  render_graph::pass_executor default_pass_executor_resolver::create_callback_executor(const std::string_view entry, const pipeline_pass_definition def, render_pipeline* pl) {
    if (system == nullptr || subsystem<scripting_environment>::inert) {
      return nullptr;
    }
    return [this, e = std::string{ entry }, p = pl](renderer& r, render_graph::node* n, void* ud) {
      auto* env = subsystem<scripting_environment>::get();
      OTHER_ASSERT(env != nullptr, "scripting_environment null in dotnet_make_pass_executor!");
      pass_invocation_interop interop{
        .renderer_ptr = &r,
        .node = n,
        .pipeline = p,
        .params = nullptr,
        .params_size = 0,
      };
      system->get_driver().get_interface_registry().invoke_callback<>(e, &interop);
    };
  }

  resource_tag_binder_fn_t default_pass_executor_resolver::create_callback_tag_binder(const std::string_view entry, resource_tag tag) {
    if (system == nullptr || subsystem<scripting_environment>::inert) {
      return nullptr;
    }
    return [this, e = std::string{ entry }](render_pipeline& p, const render_data& d, resource_handle h) {
      CORE_LOG_ERROR("Invoking resource tag binder callback [{}] on resource handle [{}]", e, h.id);
    };
  }

}  // namespace other