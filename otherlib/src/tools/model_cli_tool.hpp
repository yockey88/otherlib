/**
 * \file tools/model_cli_tool.hpp
 *
 * `oecli model info <file>` — imports a model headless through model_importer::import
 * (pure cpu, no gpu or job system) and prints its geometry/material/skeleton/clip summary.
 * `oecli model extract-clips <file> [-o <dir>]` — writes each embedded animation clip to
 * its own `<stem>@<clip>.oanim` beside the model; the authoring loop for animation
 * graphs is import once, extract, reference the .oanim files.
 * `oecli anim info <file.oanim>` — parses a standalone clip and prints its track summary.
 * `model bake` is deliberately absent until the .omdl writer exists.
 *
 * lives in otherlib because it needs the cli tool interface (other_cli), the model
 * importer (other_renderer), and the .oanim codec (other_scene).
 **/
#ifndef OTHERLIB_TOOLS_MODEL_CLI_TOOL_HPP
#define OTHERLIB_TOOLS_MODEL_CLI_TOOL_HPP

#include "cli/tool.hpp"
#include "cli/tool_registry.hpp"

namespace other {
  namespace cli {

    class model_tool final : public tool {
     public:
      model_tool() = default;
      ~model_tool() override = default;

      std::string_view name() const override { return "model"; }
      std::string_view summary() const override { return "Inspect model files and extract their animation clips (.fbx/.obj/.gltf/.glb/.dae/.3ds)"; }
      std::string_view usage() const override;

      tool_result execute(tool_context& ctx, std::span<const std::string> args) override;
    };

    class anim_tool final : public tool {
     public:
      anim_tool() = default;
      ~anim_tool() override = default;

      std::string_view name() const override { return "anim"; }
      std::string_view summary() const override { return "Inspect standalone animation clips (.oanim)"; }
      std::string_view usage() const override;

      tool_result execute(tool_context& ctx, std::span<const std::string> args) override;
    };

  }  // namespace cli
}  // namespace other

#endif  // OTHERLIB_TOOLS_MODEL_CLI_TOOL_HPP
