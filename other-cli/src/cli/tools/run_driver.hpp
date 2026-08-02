/**
 * \file cli/tools/run_driver.hpp
 **/
#ifndef OTHER_CLI_TOOLS_RUN_DRIVER_HPP
#define OTHER_CLI_TOOLS_RUN_DRIVER_HPP

#include "cli/tool.hpp"

namespace other {
  namespace cli {

    /// launches one of the source-tree development drivers (editor, server, scratch)
    ///  with its canonical config file; `open` remains the project-oriented launcher
    class run_driver_tool final : public tool {
     public:
      run_driver_tool() = default;
      ~run_driver_tool() override = default;

      std::string_view name() const override { return "run"; }
      std::string_view summary() const override { return "Launch a source-tree driver (editor, server, scratch)"; }
      std::string_view usage() const override;

      tool_result execute(tool_context& ctx, std::span<const std::string> args) override;
    };

  }  // namespace cli
}  // namespace other

#endif  // OTHER_CLI_TOOLS_RUN_DRIVER_HPP
