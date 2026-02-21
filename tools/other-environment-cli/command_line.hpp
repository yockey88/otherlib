/**
 * \file other-environment-cli/command_line.hpp
 **/
#ifndef OTHER_ENVIRONMENT_CLI_COMMAND_LINE_HPP
#define OTHER_ENVIRONMENT_CLI_COMMAND_LINE_HPP

#include <array>
#include <optional>
#include <string_view>

struct argument_definition {
  std::string_view short_flag;
  std::string_view long_flag;
  std::string_view description;
  std::string_view short_usage;

  // clang-format off
  constexpr argument_definition(const std::string_view short_flag, const std::string_view long_flag, 
                                const std::string_view description, std::optional<const std::string_view> short_usage = std::nullopt)
      : short_flag(short_flag), long_flag(long_flag), description(description), short_usage(short_usage.value_or("")) {}
  // clang-format on
};

static constexpr std::string_view kProjectUsage =
  R"(
  -p, project <command [settings]>    Specify a project file to load on startup.  
    Commands:
      create <project_name> <project_path>  Create a new project with the given name at the given path.
      edit <project_path>                   Open the specified project file in the editor.
      build <project_path>                  Build the specified project file.
      clean <project_path>                  Clean the build artifacts for the specified project file.
      run <project_path>                    Build and run the specified project file.
    Options:
      -g, game                           [create] option specifying game template
      -t, template <template-path>       [create] option specifying project template stored at the given path
)";

static constexpr std::array<argument_definition, 3> kArgumentDefinitions = {
  argument_definition("-h", "help", "Show this help message and exit"),
  argument_definition("-v", "version", "Show version information and exit"),
  argument_definition("-p", "project", kProjectUsage, "<command [options]>")
};

static constexpr std::string_view kVersion = "0.1.0";

#endif  // OTHER_ENVIRONMENT_CLI_COMMAND_LINE_HPP