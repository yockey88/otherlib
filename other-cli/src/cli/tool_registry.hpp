/**
 * \file cli/tool_registry.hpp
 **/
#ifndef OTHER_CLI_TOOL_REGISTRY_HPP
#define OTHER_CLI_TOOL_REGISTRY_HPP

#include <memory>
#include <vector>

#include "cli/tool.hpp"

namespace other {
  namespace cli {

    /// name -> tool table; the same registry backs the standalone executable and any
    ///  in-environment host (editor console, driver code, plugins)
    /// \note tools are held in std::unique_ptr rather than other::scope because scope<>
    ///  allocates through the arena subsystem and the cli must run without engine boot
    class tool_registry {
     public:
      tool_registry() = default;
      ~tool_registry() = default;

      tool_registry(tool_registry&&) = default;
      tool_registry& operator=(tool_registry&&) = default;
      tool_registry(const tool_registry&) = delete;
      tool_registry& operator=(const tool_registry&) = delete;

      /// registration is programmer-driven, so a null tool or duplicate name is a contract
      ///  violation, not a user error
      void add_tool(std::unique_ptr<tool> new_tool);

      tool* find_tool(std::string_view name) const;
      std::span<const std::unique_ptr<tool>> tools() const;

      tool_result execute(tool_context& ctx, std::string_view name, std::span<const std::string> args) const;

      /// console-style entry point: "create my-game --dir C:/projects"
      tool_result execute_line(tool_context& ctx, std::string_view line) const;

     private:
      std::vector<std::unique_ptr<tool>> registered_tools;
    };

    /// process-wide registry with the builtin project-workflow tools (create, open)
    ///  pre-registered; developer-facing hosts layer the source-tree workflow on top
    ///  through register_dev_tools
    tool_registry& default_tool_registry();

    /// registers the developer/source-tree workflow tools (run, build, test, install,
    ///  package); idempotent so the dev cli front end and driver boot can both call it.
    ///  The user cli that ships with the SDK never does, which is what differentiates
    ///  the two flavors
    void register_dev_tools(tool_registry& registry);

    /// splits a raw command line into whitespace-separated arguments; segments wrapped in
    ///  single or double quotes keep their whitespace (quotes are stripped, no escapes)
    std::vector<std::string> split_command_line(std::string_view line);

    /// convenience entry points for invoking tools from inside a running environment;
    ///  the context-less overload prints to stdout/stderr and locates the environment
    ///  relative to the current process
    tool_result run(tool_context& ctx, std::string_view command_line);
    tool_result run(std::string_view command_line);

  }  // namespace cli
}  // namespace other

#endif  // OTHER_CLI_TOOL_REGISTRY_HPP
