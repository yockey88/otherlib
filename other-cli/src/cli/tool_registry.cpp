/**
 * \file cli/tool_registry.cpp
 **/
#include "cli/tool_registry.hpp"

#include <algorithm>
#include <filesystem>
#include <print>

#include "core/logger.hpp"

#include "cli/tools/build_environment.hpp"
#include "cli/tools/create_project.hpp"
#include "cli/tools/install_environment.hpp"
#include "cli/tools/open_project.hpp"
#include "cli/tools/run_driver.hpp"
#include "cli/tools/test_runner.hpp"

namespace other {
  namespace cli {

    void tool_registry::add_tool(std::unique_ptr<tool> new_tool) {
      OTHER_ASSERT(new_tool != nullptr, "Attempted to register a null cli tool.");
      OTHER_ASSERT(!new_tool->name().empty(), "Attempted to register a cli tool with an empty name.");
      OTHER_ASSERT(find_tool(new_tool->name()) == nullptr, "A cli tool named '{}' is already registered.", new_tool->name());
      registered_tools.push_back(std::move(new_tool));
    }

    tool* tool_registry::find_tool(std::string_view name) const {
      auto it = std::ranges::find_if(registered_tools, [&name](const std::unique_ptr<tool>& t) { return t->name() == name; });
      return it != registered_tools.end() ? it->get() : nullptr;
    }

    std::span<const std::unique_ptr<tool>> tool_registry::tools() const {
      return registered_tools;
    }

    tool_result tool_registry::execute(tool_context& ctx, std::string_view name, std::span<const std::string> args) const {
      tool* t = find_tool(name);
      if (t == nullptr) {
        std::string known = "";
        for (const std::unique_ptr<tool>& registered : registered_tools) {
          known += std::format("{}{}", known.empty() ? "" : ", ", registered->name());
        }
        return tool_result::error(std::format("unknown tool '{}' (available: {})", name, known));
      }
      return t->execute(ctx, args);
    }

    tool_result tool_registry::execute_line(tool_context& ctx, std::string_view line) const {
      std::vector<std::string> parts = split_command_line(line);
      if (parts.empty()) {
        return tool_result::error("no tool given");
      }
      return execute(ctx, parts[0], std::span(parts).subspan(1));
    }

    tool_registry& default_tool_registry() {
      static tool_registry registry = [] {
        tool_registry tools;
        /// only the project workflow is builtin; the developer/source-tree workflow is
        ///  opt-in per host (dev cli front end, driver boot) through register_dev_tools,
        ///  so the user cli that ships with the SDK never carries it
        tools.add_tool(std::make_unique<create_project_tool>());
        tools.add_tool(std::make_unique<open_project_tool>());
        return tools;
      }();
      return registry;
    }

    void register_dev_tools(tool_registry& registry) {
      if (registry.find_tool("build") != nullptr) {
        return;
      }
      registry.add_tool(std::make_unique<run_driver_tool>());
      registry.add_tool(std::make_unique<build_environment_tool>());
      registry.add_tool(std::make_unique<test_runner_tool>());
      registry.add_tool(std::make_unique<install_environment_tool>());
      registry.add_tool(std::make_unique<package_environment_tool>());
    }

    std::vector<std::string> split_command_line(std::string_view line) {
      std::vector<std::string> parts;
      std::string current = "";
      bool in_token = false;
      char quote = '\0';

      for (const char c : line) {
        if (quote != '\0') {
          if (c == quote) {
            quote = '\0';
          } else {
            current.push_back(c);
          }
          continue;
        }

        if (c == '"' || c == '\'') {
          quote = c;
          in_token = true;
          continue;
        }

        if (std::isspace(static_cast<unsigned char>(c)) != 0) {
          if (in_token) {
            parts.push_back(std::move(current));
            current.clear();
            in_token = false;
          }
          continue;
        }

        current.push_back(c);
        in_token = true;
      }

      if (in_token) {
        parts.push_back(std::move(current));
      }
      return parts;
    }

    tool_result run(tool_context& ctx, std::string_view command_line) {
      return default_tool_registry().execute_line(ctx, command_line);
    }

    tool_result run(std::string_view command_line) {
      tool_context ctx;
      ctx.working_directory = std::filesystem::current_path();
      ctx.env = locate_environment();
      ctx.out = [](std::string_view message) { std::println("{}", message); };
      ctx.err = [](std::string_view message) { std::println(stderr, "{}", message); };
      return run(ctx, command_line);
    }

  }  // namespace cli
}  // namespace other
