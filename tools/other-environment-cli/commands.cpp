/**
 * \file other-environment-cli/commands.cpp
 **/
#include "commands.hpp"

#include <iostream>
#include <print>
#include <ranges>

#include "core/fnv.hpp"

#include "command_line.hpp"

other::opt<oecli_command> parse_command_line_arguments(int argc, char* argv[]) {
  oecli_command command;

  std::vector<std::string> args =
    std::span<char*>(argv, static_cast<size_t>(argc)) | std::views::drop(1) |
    std::views::transform([](char* arg) { return std::string((const char*)arg, std::strlen(arg)); }) |
    // trim whitspace on ends
    std::views::transform([](const std::string& arg) {
      return arg |
        std::views::drop_while([](char c) { return std::isspace(static_cast<unsigned char>(c)); }) | std::views::reverse |
        std::views::drop_while([](char c) { return std::isspace(static_cast<unsigned char>(c)); }) | std::views::reverse |
        std::ranges::to<std::string>();
    }) |
    std::ranges::to<std::vector>();

  auto matching_arg = std::ranges::find_if(kArgumentDefinitions, [&args](const argument_definition& arg_def) {
    return !args.empty() && (args[0] == arg_def.short_flag || args[0] == arg_def.long_flag);
  });
  if (matching_arg != kArgumentDefinitions.end()) {
    command.name = matching_arg->long_flag;
    command.args = args | std::views::drop(1) | std::ranges::to<std::vector>();
  } else {
    std::println(std::cerr, "Error: Unrecognized command '{}'", args[0]);
    return std::nullopt;
  }

  return command;
}

namespace detail {

  enum project_flags : int {
    NONE = 0,
    PROJECT_TEMPLATE = 1 << 0,
    GAME_TEMPLATE = 1 << 1,
  };

  struct file_template {
    std::string_view name;
    std::string_view path;
  };

  struct project_template {
    struct templated_file {
      std::string template_name;
      std::string destination_path;
      std::vector<std::pair<std::string, std::string>> template_variables;
    };

    std::vector<std::string> directories_to_make;
    std::vector<templated_file> files_to_template;
  };

  int project_command(const oecli_command& command);
  int create_project(const std::vector<std::string>& args);
  int edit_project(const std::vector<std::string>& args);
  int build_project(const std::vector<std::string>& args);
  int clean_project(const std::vector<std::string>& args);
  int run_project(const std::vector<std::string>& args);

  static constexpr std::array<file_template, 6> kDefaultProjectTemplateFiles = {
    file_template{ "app-driver-header", "templates/app-driver.hpp" },
    file_template{ "app-driver-source", "templates/app-driver.cpp" },
    file_template{ "environment-config", "templates/app-environment-config.toml" },
    file_template{ "app-main-source", "templates/app.cpp" },
    file_template{ "app-cmake", "templates/CMakeLists.application.txt" },
    file_template{ "project-cmake", "templates/CMakeLists.project.txt" }
  };

}  // namespace detail

int handle_command(const oecli_command& command) {
  switch (other::FNV(command.name)) {
    case other::FNV("project"): return detail::project_command(command);
    default:
      std::println(std::cerr, "Error: Unhandled command '{}'", command.name);
      return -1;
  }
}

namespace detail {

  int project_command(const oecli_command& command) {
    if (command.args.empty() || command.args[0].empty()) {
      std::println(std::cerr, "Error: No project command specified.");
      return -1;
    }

    auto command_name = command.args[0];
    auto args = command.args | std::views::drop(1) | std::ranges::to<std::vector>();

    switch (other::FNV(command_name)) {
      case other::FNV("create"): return create_project(args);
      case other::FNV("edit"): return edit_project(args);
      case other::FNV("build"): return build_project(args);
      case other::FNV("clean"): return clean_project(args);
      case other::FNV("run"): return run_project(args);
      default:
        std::println(std::cerr, "Error: Unrecognized project command '{}'", command.args.empty() ? "" : command.args[0]);
        return -1;
    }
    return 0;
  }

  int create_project(const std::vector<std::string>& args) {
    int flags = NONE;

    other::filepath temp_path = "";

    auto template_it = std::ranges::find(args, "-t");
    auto game_it = std::ranges::find(args, "-g");
    if (template_it == args.end()) {
      template_it = std::ranges::find(args, "--template");
    }

    if (template_it != args.end()) {
      if (game_it != args.end()) {
        std::println(std::cerr, "Error: Cannot specify both a game template and a custom template.");
        return -1;
      }

      bool next_exists = template_it != args.end() && std::next(template_it) != args.end();
      if (next_exists) {
        temp_path = *(std::next(template_it));
      }

      if (next_exists && !std::filesystem::exists(temp_path)) {
        temp_path = std::filesystem::absolute(temp_path);
        if (!std::filesystem::exists(temp_path)) {
          std::println(std::cerr, "Error: Template path '{}' does not exist.", temp_path.string());
          return -1;
        }
      }
    } else if (game_it == args.end()) {
      game_it = std::ranges::find(args, "--game");
    }

    /// \todo
    /// project_template templ = {}
    if (template_it != args.end()) {
      flags |= PROJECT_TEMPLATE;
      /// \todo
      /// templ = get_project_template(temp_path);
      /// validate_project_template(templ);
    } else if (game_it != args.end()) {
      flags |= GAME_TEMPLATE;
      /// \todo
      /// templ = get_game_template();
    }

    /// for now we just build the default template
    std::vector<std::string> dirs = {
      "assets",
      "src"
    };

    return -1;
  }

  int edit_project(const std::vector<std::string>& args) {
    return -1;
  }

  int build_project(const std::vector<std::string>& args) {
    return -1;
  }

  int clean_project(const std::vector<std::string>& args) {
    return -1;
  }

  int run_project(const std::vector<std::string>& args) {
    return -1;
  }

}  // namespace detail