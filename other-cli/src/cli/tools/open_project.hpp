/**
 * \file cli/tools/open_project.hpp
 **/
#ifndef OTHER_CLI_TOOLS_OPEN_PROJECT_HPP
#define OTHER_CLI_TOOLS_OPEN_PROJECT_HPP

#include "cli/tool.hpp"

namespace other {
  namespace cli {

    /// resolves a project file (given directly, or discovered in a directory) and launches
    ///  the environment's editor with it
    class open_project_tool final : public tool {
     public:
      open_project_tool() = default;
      ~open_project_tool() override = default;

      std::string_view name() const override { return "open"; }
      std::string_view summary() const override { return "Open a project in the Other Environment editor"; }
      std::string_view usage() const override;

      tool_result execute(tool_context& ctx, std::span<const std::string> args) override;
    };

  }  // namespace cli
}  // namespace other

#endif  // OTHER_CLI_TOOLS_OPEN_PROJECT_HPP
