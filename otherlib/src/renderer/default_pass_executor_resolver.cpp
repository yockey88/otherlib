/**
 * \file renderer/default_pass_executor_resolver.cpp
 **/
#include "renderer/default_pass_executor_resolver.hpp"

#include "renderer/pass_executor_interop.hpp"
#include "script/scripting_environment.hpp"

namespace other {

  render_graph::pass_executor default_pass_executor_resolver::resolve_executor(std::string_view target, const pipeline_pass_definition& def, render_pipeline* pl) {
    // target = "cs:Game.Pipelines.Foo", "lua:mod.fn", "vm:prog@label"
    auto colon = target.find(':');
    OTHER_ASSERT(colon != std::string_view::npos, "script target '{}' missing host prefix", target);
    std::string_view host = target.substr(0, colon);
    std::string_view entry = target.substr(colon + 1);

    if (host == "cs") {
      return dotnet_make_pass_executor(entry, def, pl);
    }

    if (host == "lua") {
      return lua_make_pass_executor(entry, def, pl);
    }

    if (host == "vm") {
      return vm_make_pass_executor(entry, def, pl);
    }

    return nullptr;
  }

  resource_tag_binder_fn_t default_pass_executor_resolver::resolve_binder(std::string_view target, resource_tag tag) {
    auto colon = target.find(':');
    OTHER_ASSERT(colon != std::string_view::npos, "script taget '{}' missing host prefix", target);
    std::string_view host = target.substr(0, colon);
    std::string_view entry = target.substr(colon + 1);

    if (host == "cs") {
      return dotnet_make_resource_tag_binder(entry, tag);
    }

    if (host == "lua") {
      return lua_make_resource_tag_binder(entry, tag);
    }

    if (host == "vm") {
      return vm_make_resource_tag_binder(entry, tag);
    }

    return nullptr;
  }

  // name will be arbitrary number of namespace/class names and then '.Method()'
  // Example.Class.Method()
  std::string default_pass_executor_resolver::get_dotnet_class_name(const std::string_view entry) {
    std::string e{ entry };
    auto last_dot = e.find_last_of('.');
    OTHER_ASSERT(last_dot != std::string::npos, "Inalid .NET class name in pass resolver!");
    return e.substr(0, last_dot);
  }

  std::string default_pass_executor_resolver::get_dotnet_method_name(const std::string_view entry) {
    std::string e{ entry };
    auto last_dot = e.find_last_of('.');
    OTHER_ASSERT(last_dot != std::string::npos, "Inalid .NET class name in pass resolver!");
    return e.substr(last_dot + 1, e.size() - (last_dot + 1));
  }

  render_graph::pass_executor default_pass_executor_resolver::dotnet_make_pass_executor(const std::string_view entry, const pipeline_pass_definition def, render_pipeline* pl) {
    if (!scripts_enabled) {
      return nullptr;
    }
    std::string entry_str{ entry };
    return [this, e = entry_str, p = pl](renderer& r, render_graph::node* n, void* ud) {

    };
  }

  render_graph::pass_executor default_pass_executor_resolver::lua_make_pass_executor(const std::string_view entry, const pipeline_pass_definition def, render_pipeline* pl) {
    if (!scripts_enabled) {
      return nullptr;
    }
    return nullptr;
  }

  render_graph::pass_executor default_pass_executor_resolver::vm_make_pass_executor(const std::string_view entry, const pipeline_pass_definition def, render_pipeline* pl) {
    return nullptr;
  }

  resource_tag_binder_fn_t default_pass_executor_resolver::dotnet_make_resource_tag_binder(const std::string_view entry, resource_tag tag) {
    return nullptr;
  }

  resource_tag_binder_fn_t default_pass_executor_resolver::lua_make_resource_tag_binder(const std::string_view entry, resource_tag tag) {
    return nullptr;
  }

  resource_tag_binder_fn_t default_pass_executor_resolver::vm_make_resource_tag_binder(const std::string_view entry, resource_tag tag) {
    return nullptr;
  }

}  // namespace other