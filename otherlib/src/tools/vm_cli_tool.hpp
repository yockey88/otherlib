/**
 * \file tools/vm_cli_tool.hpp
 * oecli vm <build|info>: links .oasm sources into one .obin and reads a .obin header
 **/
#ifndef OTHERLIB_TOOLS_VM_CLI_TOOL_HPP
#define OTHERLIB_TOOLS_VM_CLI_TOOL_HPP

#include "cli/tool.hpp"

namespace other {

  class command_bus;

  namespace cli {

    /// the bus a tool links a program against: the live one inside a running environment,
    ///  which also holds the devices its plugins added, and otherwise local_bus with the
    ///  builtin devices put on it
    const command_bus& tool_command_bus(command_bus& local_bus);

    class vm_tool final : public tool {
     public:
      vm_tool() = default;
      ~vm_tool() override = default;

      std::string_view name() const override { return "vm"; }
      std::string_view summary() const override { return "Build and inspect VM programs (.oasm, .obin)"; }
      std::string_view usage() const override;

      tool_result execute(tool_context& ctx, std::span<const std::string> args) override;
    };

  }  // namespace cli
}  // namespace other

#endif  // OTHERLIB_TOOLS_VM_CLI_TOOL_HPP
