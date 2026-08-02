/**
 * \file cli/tools/install_environment.hpp
 *   The two distribution tools: `install` drives cmake --install into an SDK prefix,
 *   `package` drives cpack into distributable artifacts (zip, NSIS installer).
 **/
#ifndef OTHER_CLI_TOOLS_INSTALL_ENVIRONMENT_HPP
#define OTHER_CLI_TOOLS_INSTALL_ENVIRONMENT_HPP

#include "cli/tool.hpp"

namespace other {
  namespace cli {

    class install_environment_tool final : public tool {
     public:
      install_environment_tool() = default;
      ~install_environment_tool() override = default;

      std::string_view name() const override { return "install"; }
      std::string_view summary() const override { return "Install the built environment SDK (cmake --install)"; }
      std::string_view usage() const override;

      tool_result execute(tool_context& ctx, std::span<const std::string> args) override;
    };

    class package_environment_tool final : public tool {
     public:
      package_environment_tool() = default;
      ~package_environment_tool() override = default;

      std::string_view name() const override { return "package"; }
      std::string_view summary() const override { return "Package the built environment into distributable artifacts (cpack)"; }
      std::string_view usage() const override;

      tool_result execute(tool_context& ctx, std::span<const std::string> args) override;
    };

  }  // namespace cli
}  // namespace other

#endif  // OTHER_CLI_TOOLS_INSTALL_ENVIRONMENT_HPP
