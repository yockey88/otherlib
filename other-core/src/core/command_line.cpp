/**
 * \file core/command_line.cpp
 **/
#include "core/command_line.hpp"

#include <print>

#define ARGS_NOEXCEPT
#include <args.hxx>

#include "core/defines.hpp"
#include "core/logger.hpp"

namespace other {
  namespace {

    opt<command_line> parse_raw_args(int* argc, char* argv[]) {
      args::ArgumentParser parser("Other-Environment Options", "");
      args::Positional<std::string> config_file(parser, "config-file", "Configuration file");

      args::HelpFlag help(parser, "help", "Display this help message", { 'h', "help" });
      args::Flag verbose(parser, "verbose", "Enable verbose output", { 'v', "verbose" });
      args::ValueFlag<integer_t> session_id(parser, "session-id", "Session ID to use when checking in with the server", { 's', "session-id", "sid" });
      args::ValueFlag<uint16_t> port(parser, "port", "Port to use to check in with the server, if not used, then check-in is attempted at port 49222", { 'p', "port" }, 49222);

      parser.ParseCLI(*argc, argv);
      args::Error err = parser.GetError();

      switch (err) {
        case args::Error::None:
          break;

        case args::Error::Help:
          std::cout << parser;
          return command_line{ .diagnostics = { .help = true } };

        case args::Error::Usage:
          std::cout << parser;
          return command_line{ .diagnostics = { .usage = true } };

        default:
          std::println(std::cerr, "Command Line Error [{}] : {}", err, parser.GetErrorMsg());
          std::nullopt;
      }

      command_line cmd;
      cmd.diagnostics.verbose = verbose.Get();
      cmd.config_file = config_file.Get();

      if (session_id) {
        cmd.session_id = session_id.Get();
      }
      if (port) {
        cmd.port = port.Get();
      }

      if (cmd.diagnostics.verbose) {
        std::println(std::cout, "Verbose output: {}", cmd.diagnostics.verbose ? "enabled" : "disabled");
        std::println(std::cout, "Using configuration file: '{}'", cmd.config_file);
        std::println(std::cout, "Session ID: {}", cmd.session_id.has_value() ? std::to_string(cmd.session_id.value()) : "not specified");
        std::println(std::cout, "Port: {}", cmd.port.value());
      }

      cmd.valid = true;
      return cmd;
    }

  }  // namespace

  command_line command_line::parse(int* argc, char* argv[]) {
    return parse_raw_args(argc, argv).value_or(command_line{});
  }

}  // namespace other