/**
 * \file tools/scene_cli_tool.hpp
 * oecli scene <compile|decompile|info>: .oscn/.oscnb conversions on the scene_document
 *  layer only (no engine subsystems boot); reachable in-process via other::cli::run(...)
 **/
#ifndef OTHERLIB_TOOLS_SCENE_CLI_TOOL_HPP
#define OTHERLIB_TOOLS_SCENE_CLI_TOOL_HPP

#include "cli/tool.hpp"
#include "cli/tool_registry.hpp"

namespace other {
  namespace cli {

    class scene_tool final : public tool {
     public:
      scene_tool() = default;
      ~scene_tool() override = default;

      std::string_view name() const override { return "scene"; }
      std::string_view summary() const override { return "Compile, decompile, and inspect scene documents (.oscn/.oscnb)"; }
      std::string_view usage() const override;

      tool_result execute(tool_context& ctx, std::span<const std::string> args) override;
    };

    /// registers engine-level tools (scene, model, ...) the core cli library can't host
    ///  itself; idempotent so any frontend can call it unconditionally
    void register_environment_tools(tool_registry& registry);

    /// engine-level tools need the arena+logger live; if no host booted them (bare oecli)
    ///  this activates them console-warn-only. no-op inside a running environment
    void ensure_cli_runtime();

  }  // namespace cli
}  // namespace other

#endif  // OTHERLIB_TOOLS_SCENE_CLI_TOOL_HPP
