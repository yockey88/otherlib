/**
 * \file tools/compile_cli_tool.hpp
 * oecli compile <project>: package a project as a folder that runs on its own
 **/
#ifndef OTHERLIB_TOOLS_COMPILE_CLI_TOOL_HPP
#define OTHERLIB_TOOLS_COMPILE_CLI_TOOL_HPP

#include "cli/tool.hpp"

namespace other {
  namespace cli {

    class compile_tool final : public tool {
     public:
      compile_tool() = default;
      ~compile_tool() override = default;

      std::string_view name() const override { return "compile"; }
      std::string_view summary() const override { return "Package a project as a folder that runs on its own"; }
      std::string_view usage() const override;

      tool_result execute(tool_context& ctx, std::span<const std::string> args) override;
    };

  }  // namespace cli
}  // namespace other

#endif  // OTHERLIB_TOOLS_COMPILE_CLI_TOOL_HPP
