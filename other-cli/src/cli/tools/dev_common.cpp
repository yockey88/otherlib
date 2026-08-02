/**
 * \file cli/tools/dev_common.cpp
 **/
#include "cli/tools/dev_common.hpp"

#include <filesystem>
#include <format>

namespace other {
  namespace cli {
    namespace {

#ifdef OTHER_ENVIRONMENT_WINDOWS
      constexpr std::string_view kExecutableSuffix = ".exe";
#else
      constexpr std::string_view kExecutableSuffix = "";
#endif

    }  // namespace

    opt<tool_result> try_parse_dev_flag(tool_context& ctx, std::span<const std::string> args, size_t& i, dev_tool_options& options) {
      const std::string& arg = args[i];
      auto flag_value = [&args, &i]() -> opt<std::string> {
        if (i + 1 >= args.size()) {
          return std::nullopt;
        }
        return args[++i];
      };

      if (arg == "--config" || arg == "-c") {
        opt<std::string> value = flag_value();
        if (!value.has_value()) {
          return tool_result::error(std::format("'{}' requires a build config value", arg));
        }
        if (!is_valid_build_config(value.value())) {
          return tool_result::error(std::format("invalid build config '{}' (expected Debug, Release, Profile, or ProfileD)", value.value()));
        }
        options.config = value;
        return tool_result::ok();
      }

      if (arg == "--env-root") {
        opt<std::string> value = flag_value();
        if (!value.has_value()) {
          return tool_result::error(std::format("'{}' requires a path value", arg));
        }
        ctx.env = locate_environment(filepath(value.value()));
        if (!ctx.env.found) {
          return tool_result::error(std::format("'{}' is not an Other Environment root", value.value()));
        }
        return tool_result::ok();
      }

      if (arg == "--dry-run") {
        options.dry_run = true;
        return tool_result::ok();
      }

      return std::nullopt;
    }

    tool_result require_source_tree(const tool_context& ctx, std::string_view verb) {
      if (!ctx.env.found) {
        return tool_result::error(
          "no Other Environment found (set OTHER_ENVIRONMENT_ROOT, pass --env-root, or run from inside an environment tree)");
      }
      if (!ctx.env.in_source_tree) {
        return tool_result::error(
          std::format("cannot {} from the installed SDK at '{}'; pass --env-root pointing at a source tree", verb, ctx.env.root.string()));
      }
      return tool_result::ok();
    }

    tool_result run_attached(const tool_context& ctx, process_launch launch, bool dry_run) {
      launch.wait_for_exit = true;
      launch.new_console = false;

      if (dry_run) {
        ctx.print("would run: {}", format_command_line(launch));
        if (!launch.working_directory.empty()) {
          ctx.print("  from cwd: {}", launch.working_directory.string());
        }
        return tool_result::ok();
      }

      ctx.print("running: {}", format_command_line(launch));
      const process_result result = launch_process(launch);
      if (!result.started) {
        return tool_result::error(std::format("failed to launch '{}': {}", launch.executable.string(), result.error));
      }
      return { .code = result.exit_code, .message = "" };
    }

    filepath find_built_executable(const environment_paths& env, const filepath& output_dir, std::string_view executable_name,
                                   const opt<std::string>& config, std::string& resolved_config) {
      const auto candidate_for = [&env, &output_dir, &executable_name](std::string_view build_config) {
        return env.root / "build" / output_dir / build_config / std::format("{}{}", executable_name, kExecutableSuffix);
      };

      if (config.has_value()) {
        filepath candidate = candidate_for(config.value());
        if (std::filesystem::exists(candidate)) {
          resolved_config = config.value();
          return candidate;
        }
        return "";
      }

      for (const std::string_view build_config : build_config_probe_order()) {
        filepath candidate = candidate_for(build_config);
        if (std::filesystem::exists(candidate)) {
          resolved_config = std::string{ build_config };
          return candidate;
        }
      }
      return "";
    }

  }  // namespace cli
}  // namespace other
