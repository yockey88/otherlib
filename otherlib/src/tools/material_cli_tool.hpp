/**
 * \file tools/material_cli_tool.hpp
 **/
#ifndef OTHERLIB_TOOLS_MATERIAL_CLI_TOOL_HPP
#define OTHERLIB_TOOLS_MATERIAL_CLI_TOOL_HPP

#include "cli/tool.hpp"

namespace other {
  namespace cli {

    class material_tool final : public tool {
     public:
      material_tool() = default;
      ~material_tool() override = default;

      std::string_view name() const override { return "material"; }
      std::string_view summary() const override { return "Inspect material files (.omat)"; }
      std::string_view usage() const override;

      tool_result execute(tool_context& ctx, std::span<const std::string> args) override;
    };

  }  // namespace cli
}  // namespace other

#endif  // OTHERLIB_TOOLS_MATERIAL_CLI_TOOL_HPP