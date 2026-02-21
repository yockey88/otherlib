/**
 * \file other-environment-cli/main.cpp
 **/
#include <iostream>
#include <print>

#include "core/fnv.hpp"

#include "command_line.hpp"
#include "commands.hpp"

int main(int argc, char* argv[]) {
  {
    bool ret_early = false;

    std::println("Other Environment CLI - A tool for managing Other Engine projects.");
    if (argc == 1 || (argc == 2 && (std::string_view(argv[1]) == "-h" || std::string_view(argv[1]) == "--help"))) {
      std::println("Usage: oecli [command [options...]]");
      std::println("Commands:");
      for (const auto& arg_def : kArgumentDefinitions) {
        std::println("  {}, {}{}", arg_def.short_flag, arg_def.long_flag, (arg_def.short_usage.empty() ? "" : std::format(": {}", arg_def.short_usage)));
      }
    }

    if (argc == 2 && (std::string_view(argv[1]) == "-h" || std::string_view(argv[1]) == "--help")) {
      std::println("\nCommand Usage:");
      for (const auto& arg_def : kArgumentDefinitions) {
        std::println("{}, {}:   {}", arg_def.short_flag, arg_def.long_flag, arg_def.description);
      }
      ret_early = true;
    } else if (argc == 2 && (std::string_view(argv[1]) == "-v" || std::string_view(argv[1]) == "--version")) {
      std::println("   v{}", kVersion);
      ret_early = true;
    }

    if (ret_early) {
      return 0;
    }
  }

  other::opt<oecli_command> command = parse_command_line_arguments(argc, argv);
  if (!command.has_value()) {
    std::println(std::cerr, "Failed to parse command line arguments.");
    return -1;
  }

  return handle_command(*command);
}
