/**
 * \file core/command_line.cpp
 **/
#include "core/command_line.hpp"

#include <print>

#define ARGS_NOEXCEPT
#include <args.hxx>

#include "core/defines.hpp"
#include "core/profiler.hpp"

namespace other {
  namespace {

    opt<command_line> parse_raw_args(int* argc, char* argv[]) {
      PROFILE_SECTION("command_line::parse_raw_args");
      args::ArgumentParser parser("Other-Environment Options", "");
      args::Positional<std::string> config_file(parser, "config-file", "Configuration file");

      args::HelpFlag help(parser, "help", "Display this help message", { 'h', "help" });
      args::Flag verbose(parser, "verbose", "Enable verbose output", { 'v', "verbose" });
      args::ValueFlag<std::string> cwd(parser, "working-directory", "Set the working directory for the application", { 'd', "cwd" });
      args::ValueFlag<integer_t> session_id(parser, "session-id", "Session ID to use when checking in with the server", { 's', "session-id", "sid" });
      args::ValueFlag<uint16_t> port(parser, "port", "Port to use to check in with the server, if not used, then check-in is attempted at port 49222", { 'p', "port" }, 49222);
      args::ValueFlag<std::string> project_file(parser, "project-file", "Path to the project file to load on startup", { 'f', "project-file" });
      args::ValueFlag<std::string> project_dir(parser, "project-dir", "Path to the a directory to load a project from or generate a project in", { 'P', "project-dir" });

      parser.ParseCLI(*argc, argv);
      args::Error err = parser.GetError();

      switch (err) {
        case args::Error::None:
          break;

        case args::Error::Help:
        case args::Error::Usage:
          std::cout << parser;
          return command_line{
            .diagnostics = {
              .help = err == args::Error::Help,
              .usage = err == args::Error::Usage,
            }
          };

        default:
          std::println(std::cerr, "Command Line Error [{}] : {}", err, parser.GetErrorMsg());
          return std::nullopt;
      }

      command_line cmd;
      cmd.diagnostics.verbose = verbose.Get();
      cmd.config_file = config_file.Get();

      bool has_project_file_arg = project_file;
      bool has_project_dir_arg = project_dir;
      if (has_project_file_arg && has_project_dir_arg) {
        std::println(std::cerr, "Cannot specify both --project-file and --project-dir arguments. Please specify only one of these.");
        return std::nullopt;
      }

      if (project_file) {
        filepath absolute_path = std::filesystem::absolute(project_file.Get());
        if (!std::filesystem::exists(absolute_path)) {
          std::println(std::cerr, "Project file '{}' does not exist.", absolute_path.string());
          return std::nullopt;
        }
        if (!std::filesystem::is_regular_file(absolute_path)) {
          std::println(std::cerr, "Project file '{}' is not a file.", absolute_path.string());
          return std::nullopt;
        }

        cmd.project_file = absolute_path;
      } else if (project_dir) {
        // filepath absolute_path = std::filesystem::absolute(project_dir.Get());
        // if (!std::filesystem::exists(absolute_path)) {
        //   std::println(std::cerr, "Project directory '{}' does not exist.", absolute_path.string());
        //   return std::nullopt;
        // }
        // if (!std::filesystem::is_directory(absolute_path)) {
        //   std::println(std::cerr, "Project directory '{}' is not a directory.", absolute_path.string());
        //   return std::nullopt;
        // }

        // // If the project directory exists but does not contain a project file, we can generate a new project file in that directory.
        // // We will generate the project file at {project_dir}/{project_name}.toml, where project_name is derived from the name of the project directory.
        // std::string project_name = absolute_path.filename().stem().string();
        // filepath generated_project_file = absolute_path / filepath(project_name + ".toml");
        // if (!std::filesystem::exists(generated_project_file)) {
        //   std::ofstream file(generated_project_file);
        //   file << "project = \"{}\"\n\n[project.metadata]\nname = \"{}\"\nauthor = \"\"\ndescription = \"\"\nversion = \"0.1.0\"\n\n[project.scripting]\ncs_project = \"{}\"\n";
        //   file.close();

        //   std::println(std::cout, "Generated new project file at '{}'", generated_project_file.string());
        // } else {
        //   std::println(std::cout, "Found existing project file at '{}'", generated_project_file.string());
        // }

        // cmd.project_file = generated_project_file;
      }

      if (session_id) {
        cmd.session_id = session_id.Get();
      }
      if (port) {
        cmd.port = port.Get();
      }
      if (cwd) {
        cmd.working_directory = cwd.Get();
      }

      if (cmd.diagnostics.verbose) {
        std::println(std::cout, "Verbose output: {}", cmd.diagnostics.verbose ? "enabled" : "disabled");
        std::println(std::cout, "Using configuration file: '{}'", cmd.config_file);
        std::println(std::cout, "Session ID: {}", cmd.session_id.has_value() ? std::to_string(cmd.session_id.value()) : "not specified");
        std::println(std::cout, "Port: {}", cmd.port.value());
        std::println(std::cout, "Working Directory: {}", cmd.working_directory.has_value() ? cmd.working_directory.value().string() : "not specified");
      }

      cmd.valid = true;
      return cmd;
    }

  }  // namespace

  command_line command_line::parse(int* argc, char* argv[]) {
    return parse_raw_args(argc, argv).value_or(command_line{});
  }

}  // namespace other