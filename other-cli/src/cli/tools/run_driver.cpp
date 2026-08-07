/**
 * \file cli/tools/run_driver.cpp
 **/
#include "cli/tools/run_driver.hpp"

#include <array>
#include <vector>

#include "cli/process.hpp"
#include "cli/tools/dev_common.hpp"
#include "core/profiler.hpp"

namespace other {
  namespace cli {
    namespace {

      constexpr std::string_view kRunUsage =
        R"(usage: run [driver] [options...]

  Launches one of the source-tree development drivers attached to this console.
  Use `open <project>` to launch the editor with a specific project instead.

  drivers:
    editor    the Other Environment editor (default)
    server    the Other Environment server
    scratch   the gl-testing scratch driver

  options:
    -c, --config <config>   build config to launch (default: first built config)
    -v, --verbose           forward --verbose to the driver
        --detach            launch in its own console and return immediately
        --env-root <path>   explicit source tree root
        --dry-run           print the launch instead of running it
     -- <args...>           everything after -- is forwarded to the driver)";

      struct driver_spec {
        std::string_view name = "";
        std::string_view output_dir = "";
        std::string_view executable = "";
        std::string_view config_file = "";
        std::vector<std::string> extra_args = {};
      };

      const driver_spec* find_driver(std::string_view name) {
        /// the server resolves its config relative to --cwd, so its config file path
        ///  stays bare while everything else points into resources/
        static const std::array<driver_spec, 3> kDrivers = { {
          { .name = "editor", .output_dir = "other-editor", .executable = "other_editor", .config_file = "resources/editor-config.toml" },
          { .name = "server", .output_dir = "other-server", .executable = "other_server", .config_file = "server-config.toml",
            .extra_args = { "--cwd", "other-server" } },
          { .name = "scratch", .output_dir = "scratch", .executable = "gl-testing", .config_file = "resources/gl-test-config.toml" },
        } };

        for (const driver_spec& spec : kDrivers) {
          if (spec.name == name) {
            return &spec;
          }
        }
        return nullptr;
      }

    }  // namespace

    std::string_view run_driver_tool::usage() const {
      return kRunUsage;
    }

    tool_result run_driver_tool::execute(tool_context& ctx, std::span<const std::string> args) {
      PROFILE_SECTION("run_driver_tool::execute");
      dev_tool_options options;
      opt<std::string> driver_name = std::nullopt;
      bool verbose = false;
      bool detach = false;
      std::vector<std::string> forwarded = {};

      for (size_t i = 0; i < args.size(); ++i) {
        if (opt<tool_result> shared = try_parse_dev_flag(ctx, args, i, options); shared.has_value()) {
          if (!shared.value().success()) {
            return shared.value();
          }
          continue;
        }

        const std::string& arg = args[i];
        if (arg == "--") {
          forwarded.assign(args.begin() + static_cast<ptrdiff_t>(i) + 1, args.end());
          break;
        } else if (arg == "--verbose" || arg == "-v") {
          verbose = true;
        } else if (arg == "--detach") {
          detach = true;
        } else if (arg.starts_with("-")) {
          return tool_result::error(std::format("unknown option '{}'\n{}", arg, kRunUsage));
        } else if (!driver_name.has_value()) {
          driver_name = arg;
        } else {
          return tool_result::error(std::format("unexpected argument '{}'\n{}", arg, kRunUsage));
        }
      }

      if (tool_result guard = require_source_tree(ctx, "run a driver"); !guard.success()) {
        return guard;
      }

      const driver_spec* spec = find_driver(driver_name.value_or("editor"));
      if (spec == nullptr) {
        return tool_result::error(std::format("unknown driver '{}' (expected editor, server, or scratch)\n{}", driver_name.value(), kRunUsage));
      }

      std::string resolved_config = "";
      const filepath executable = find_built_executable(ctx.env, spec->output_dir, spec->executable, options.config, resolved_config);
      if (executable.empty()) {
        return tool_result::error(std::format("no{} {} build found under 'build/{}' (build one with: build{})",
          options.config.has_value() ? std::format(" {}", options.config.value()) : "",
          spec->executable, spec->output_dir,
          options.config.has_value() ? std::format(" --config {}", options.config.value()) : ""));
      }

      std::vector<std::string> driver_args = { std::string{ spec->config_file } };
      if (verbose) {
        driver_args.push_back("--verbose");
      }
      driver_args.insert(driver_args.end(), spec->extra_args.begin(), spec->extra_args.end());
      driver_args.insert(driver_args.end(), forwarded.begin(), forwarded.end());

      /// drivers resolve engine resources relative to their working directory, so
      ///  everything launches from the environment root
      process_launch launch = {
        .executable = executable,
        .arguments = driver_args,
        .working_directory = ctx.env.root,
      };

      if (detach) {
        launch.wait_for_exit = false;
        launch.new_console = true;
        if (options.dry_run) {
          ctx.print("would launch: {}", format_command_line(launch));
          ctx.print("    from cwd: {}", launch.working_directory.string());
          return tool_result::ok("dry run only, nothing launched");
        }

        const process_result launched = launch_process(launch);
        if (!launched.started) {
          return tool_result::error(std::format("failed to launch {}: {}", spec->executable, launched.error));
        }
        return tool_result::ok(std::format("launched {} [{}]", spec->executable, resolved_config));
      }

      ctx.print("running {} [{}]", spec->executable, resolved_config);
      const tool_result ran = run_attached(ctx, launch, options.dry_run);
      if (options.dry_run) {
        return tool_result::ok("dry run only, nothing launched");
      }
      if (!ran.success()) {
        return { .code = ran.code, .message = std::format("{} exited with code {}", spec->executable, ran.code) };
      }
      return tool_result::ok(std::format("{} exited cleanly", spec->executable));
    }

  }  // namespace cli
}  // namespace other
