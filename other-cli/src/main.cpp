/**
 * \file main.cpp
 *   Standalone front end for the Other Environment CLI. Deliberately boots no engine
 *   subsystems so the tools stay instant and usable outside a running environment;
 *   in-environment hosts reach the same tools through other::cli::run / the registry.
 **/
#include <cstdio>
#include <filesystem>
#include <print>
#include <string>
#include <vector>

#include "core/version.hpp"

#include "tools/scene_cli_tool.hpp"

#include "cli/tool_registry.hpp"

namespace {

  constexpr std::string_view kCliVersion = "0.2.0";

  void print_usage(const other::cli::tool_registry& registry) {
    std::println("Other Environment CLI v{}", kCliVersion);
    std::println("");
    std::println("usage: oecli <tool> [arguments...]");
    std::println("");
    std::println("tools:");
    for (const auto& tool : registry.tools()) {
      std::println("  {:<10} {}", tool->name(), tool->summary());
    }
    std::println("");
    std::println("run 'oecli help <tool>' for tool usage, 'oecli --version' for versions");
  }

}  // namespace

int main(int argc, char* argv[]) {
  auto& registry = other::cli::default_tool_registry();
  other::cli::register_environment_tools(registry);
  const std::vector<std::string> args(argv + 1, argv + argc);

  if (args.empty() || args[0] == "-h" || args[0] == "--help" || args[0] == "help") {
    // help <tool> case
    if (!args.empty() && args.size() > 1) {
      other::cli::tool* tool = registry.find_tool(args[1]);
      if (tool == nullptr) {
        std::println(stderr, "error: unknown tool '{}'", args[1]);
        print_usage(registry);
        return 1;
      }
      std::println("{} - {}", tool->name(), tool->summary());
      std::println("{}", tool->usage());
    } else {
      print_usage(registry);
    }
    return 0;
  }

  if (args[0] == "-v" || args[0] == "--version") {
    std::println("oecli v{} (Other Environment v{})", kCliVersion, OTHER_ENVIRONMENT_VERSION_STRING);
    return 0;
  }

  other::cli::tool_context ctx;
  ctx.working_directory = std::filesystem::current_path();
  ctx.env = other::cli::locate_environment();
  ctx.out = [](std::string_view message) {
    std::println("{}", message);
  };
  ctx.err = [](std::string_view message) {
    std::println(stderr, "{}", message);
  };

  const other::cli::tool_result result = registry.execute(ctx, args[0], std::span(args).subspan(1));
  if (!result.message.empty()) {
    if (result.success()) {
      std::println("{}", result.message);
    } else {
      std::println(stderr, "error: {}", result.message);
    }
  }
  return result.code;
}
