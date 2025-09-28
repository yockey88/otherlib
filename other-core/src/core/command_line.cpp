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
      args::HelpFlag help(parser, "help", "Display this help message", { 'h', "help" });
      args::Flag verbose(parser, "verbose", "Enable verbose output", { 'v', "verbose" });

      args::Positional<std::string> config_file(parser, "config-file", "Configuration file");
      args::PositionalList<std::string> positional_args(parser, "driver-args", "Command line arguments to forward to the linked driver executable");

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
          CORE_LOG_ERROR("Command Line Error [{}] : {}", err, parser.GetErrorMsg());
          std::nullopt;
      }

      command_line cmd;
      cmd.diagnostics.verbose = verbose.Get();
      cmd.config_file = config_file.Get();

      cmd.args.reserve(positional_args->size());
      for (const auto& arg : positional_args) {
        cmd.args.emplace_back(arg);
      }

      cmd.valid = true;
      return cmd;
    }

  }  // namespace

  command_line command_line::parse(int* argc, char* argv[]) {
    return parse_raw_args(argc, argv).value_or(command_line{});
  }

}  // namespace other