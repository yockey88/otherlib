/**
 * \file cli/tools/build_environment.hpp
 **/
#ifndef OTHER_CLI_TOOLS_BUILD_ENVIRONMENT_HPP
#define OTHER_CLI_TOOLS_BUILD_ENVIRONMENT_HPP

#include "cli/tool.hpp"

namespace other {
  namespace cli {

    /// drives a source-tree build: cmake generation when needed, the parallel build, then
    ///  staging the vendored runtime DLLs next to every built application
    class build_environment_tool final : public tool {
     public:
      build_environment_tool() = default;
      ~build_environment_tool() override = default;

      std::string_view name() const override { return "build"; }
      std::string_view summary() const override { return "Build the Other Environment from a source tree"; }
      std::string_view usage() const override;

      tool_result execute(tool_context& ctx, std::span<const std::string> args) override;
    };

  }  // namespace cli
}  // namespace other

#endif  // OTHER_CLI_TOOLS_BUILD_ENVIRONMENT_HPP
