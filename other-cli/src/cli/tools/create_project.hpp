/**
 * \file cli/tools/create_project.hpp
 **/
#ifndef OTHER_CLI_TOOLS_CREATE_PROJECT_HPP
#define OTHER_CLI_TOOLS_CREATE_PROJECT_HPP

#include "cli/tool.hpp"

namespace other {
  namespace cli {

    /// scaffolds a new editor-managed project: project file (toml), projectrc (lua),
    ///  starter scene, and a C# script project wired to the environment's assemblies
    class create_project_tool final : public tool {
     public:
      create_project_tool() = default;
      ~create_project_tool() override = default;

      std::string_view name() const override { return "create"; }
      std::string_view summary() const override { return "Create a new Other Environment project"; }
      std::string_view usage() const override;

      tool_result execute(tool_context& ctx, std::span<const std::string> args) override;
    };

  }  // namespace cli
}  // namespace other

#endif  // OTHER_CLI_TOOLS_CREATE_PROJECT_HPP
