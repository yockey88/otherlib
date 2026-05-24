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

  bool default_pass_executor_resolver::can_resolve_executor(std::string_view name) const {
    if (system == nullptr || subsystem<scripting_environment>::inert) {
      return false;
    }

    auto colon = name.find(':');
    if (colon == std::string_view::npos) {
      return false;
    }
    std::string_view host = name.substr(0, colon);
    return host == "cs" || host == "lua" || host == "vm" || host == "plugin";
  }

  bool default_pass_executor_resolver::can_resolve_frame_binder(resource_tag tag) const {
    if (system == nullptr || subsystem<scripting_environment>::inert) {
      return false;
    }
    return true;
  }

  bool default_pass_executor_resolver::can_resolve_draw_binder(resource_tag tag) const {
    if (system == nullptr || subsystem<scripting_environment>::inert) {
      return false;
    }
    return true;
  }

  bool default_pass_executor_resolver::can_resolve_instance_binder(resource_tag tag) const {
    if (system == nullptr || subsystem<scripting_environment>::inert) {
      return false;
    }
    return true;
  }

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

  per_frame_producer_fn default_pass_executor_resolver::resolve_frame_binder(const std::string_view entry, resource_tag tag) {
    auto colon = entry.find(':');
    OTHER_ASSERT(colon != std::string_view::npos, "script target '{}' missing host prefix", entry);
    std::string_view host = entry.substr(0, colon);
    std::string_view e = entry.substr(colon + 1);

    if (host == "cs" || host == "lua" || host == "vm" || host == "plugin") {
      return create_callback_frame_binder(e, tag);
    }

    return nullptr;
  }

  per_draw_producer_fn default_pass_executor_resolver::resolve_draw_binder(const std::string_view entry, resource_tag tag) {
    auto colon = entry.find(':');
    OTHER_ASSERT(colon != std::string_view::npos, "script target '{}' missing host prefix", entry);
    std::string_view host = entry.substr(0, colon);
    std::string_view e = entry.substr(colon + 1);

    if (host == "cs" || host == "lua" || host == "vm" || host == "plugin") {
      return create_callback_draw_binder(e, tag);
    }

    return nullptr;
  }

  per_instance_producer_fn default_pass_executor_resolver::resolve_instance_binder(const std::string_view entry, resource_tag tag) {
    auto colon = entry.find(':');
    OTHER_ASSERT(colon != std::string_view::npos, "script target '{}' missing host prefix", entry);
    std::string_view host = entry.substr(0, colon);
    std::string_view e = entry.substr(colon + 1);

    if (host == "cs" || host == "lua" || host == "vm" || host == "plugin") {
      return create_callback_instance_binder(e, tag);
    }

    return nullptr;
  }

  render_graph::pass_executor default_pass_executor_resolver::create_callback_executor(const std::string_view entry, const pipeline_pass_definition def, render_pipeline* pl) {
    if (system == nullptr || subsystem<scripting_environment>::inert) {
      return nullptr;
    }
    return [this, e = std::string{ entry }, p = pl](pass_context& ctx) {
      auto* env = subsystem<scripting_environment>::get();
      OTHER_ASSERT(env != nullptr, "scripting_environment null in dotnet_make_pass_executor!");
      pass_invocation_interop interop{
        .context = &ctx,
      };
      system->get_driver().get_interface_registry().invoke_callback<>(e, &interop);
    };
  }

  per_frame_producer_fn default_pass_executor_resolver::create_callback_frame_binder(const std::string_view entry, resource_tag tag) {
    if (system == nullptr || subsystem<scripting_environment>::inert) {
      return nullptr;
    }
    return [this, e = std::string{ entry }, t = tag](render_pipeline& pl, const render_data& data, resource_handle out) {
      auto* env = subsystem<scripting_environment>::get();
      OTHER_ASSERT(env != nullptr, "scripting_environment null in create_callback_frame_binder!");
    };
  }

  per_draw_producer_fn default_pass_executor_resolver::create_callback_draw_binder(const std::string_view entry, resource_tag tag) {
    if (system == nullptr || subsystem<scripting_environment>::inert) {
      return nullptr;
    }
    return [this, e = std::string{ entry }, t = tag](const render_data& data, size_t draw_index, std::span<const uint8_t> out) {
      auto* env = subsystem<scripting_environment>::get();
      OTHER_ASSERT(env != nullptr, "scripting_environment null in create_callback_draw_binder!");
    };
  }

  per_instance_producer_fn default_pass_executor_resolver::create_callback_instance_binder(const std::string_view entry, resource_tag tag) {
    if (system == nullptr || subsystem<scripting_environment>::inert) {
      return nullptr;
    }
    return [this, e = std::string{ entry }, t = tag](const render_data& data, size_t draw_index, size_t instance_index, std::span<const uint8_t> out) {
      auto* env = subsystem<scripting_environment>::get();
      OTHER_ASSERT(env != nullptr, "scripting_environment null in create_callback_instance_binder!");
    };
  }

}  // namespace other