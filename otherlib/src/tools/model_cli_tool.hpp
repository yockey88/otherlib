/**
 * \file tools/model_cli_tool.hpp
 *
 * `oecli model info <file>` — imports a model headless through model_importer::import
 * (pure cpu, no gpu or job system) and prints its geometry/material/skeleton summary.
 * `model bake` is deliberately absent until the .omdl writer exists (doc 01 section 7).
 *
 * lives in otherlib because it needs both the cli tool interface (other_cli) and the
 * model importer (other_renderer).
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
      std::string_view summary() const override { return "Inspect model files (.fbx/.obj/.gltf/.glb/.dae/.3ds)"; }
      std::string_view usage() const override;

      tool_result execute(tool_context& ctx, std::span<const std::string> args) override;
    };

  }  // namespace cli
}  // namespace other

#endif  // OTHERLIB_TOOLS_MODEL_CLI_TOOL_HPP
