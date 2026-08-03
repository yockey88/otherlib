/**
 * \file tools/scene_cli_tool.hpp
 *
 * `oecli scene <compile|decompile|info>` — document-level scene conversions between
 * .oscn (toml) and .oscnb (binary). runs entirely on the scene_document layer, so no
 * engine subsystems boot; the same tool is reachable in-process through
 * other::cli::run("scene compile ...") from the editor console or driver code.
 *
 * lives in otherlib because it needs both the cli tool interface (other_cli) and the
 * scene component codecs (other_scene) — this is the first layer that links both.
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

    /// registers the engine-level tools (scene, model, ...) that the core cli library
    ///  cannot host itself; idempotent so every frontend (oecli main, driver boot, tests)
    ///  can call it unconditionally
    void register_environment_tools(tool_registry& registry);

    /// engine-level tools allocate through the arena and log through the logger; when no
    ///  host booted those subsystems (bare oecli), this activates them console-warn-only.
    ///  inside a running environment it is a no-op.
    void ensure_cli_runtime();

  }  // namespace cli
}  // namespace other

#endif  // OTHERLIB_TOOLS_SCENE_CLI_TOOL_HPP
