/**
 * \file tools/model_cli_tool.hpp
 * oecli model/anim tools: headless model info + per-clip .oanim extraction
 *  (<stem>@<clip>.oanim); no model bake yet (needs the .omdl writer)
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
