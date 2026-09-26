/**
 * \file cli/tools/open_project.cpp
 **/
#include "cli/tools/open_project.hpp"

#include <filesystem>
#include <vector>

#include <toml++/toml.hpp>

#include "core/profiler.hpp"

#include "cli/process.hpp"

namespace other {
  namespace cli {
    namespace {

      constexpr std::string_view kOpenUsage =
        R"(usage: open [project] [options...]

  Opens a project in the Other Environment editor. [project] may be a project file
  (.toml), a directory containing one, or omitted to search the working directory.

  options:
    -c, --config <config>   editor build config: Debug, Release, Profile, ProfileD
                            (default: this binary's config, then any built config;
                            an installed SDK resolves configs to its debug/release family)
        --wait              stay attached and forward the editor's exit code
        --dry-run           print the launch command instead of running it
        --env-root <path>   explicit Other Environment root to launch from)";

      struct editor_location {
        filepath executable = "";
        std::string label = "";
      };

      bool is_project_file(const filepath& file) {
        if (file.extension() != ".toml") {
          return false;
        }
        try {
          toml::table table = toml::parse_file(file.string());
          return table.contains("project");
        } catch (...) {
          return false;
        }
      }

      filepath editor_search_root(const environment_paths& env) {
        if (env.in_source_tree) {
          return env.root / "build" / "other-editor";
        }
        return env.root / "bin";
      }

      std::string editor_missing_hint(const environment_paths& env, const opt<std::string>& config) {
        const std::string config_flag = config.has_value() ? std::format(" -c {}", config.value()) : "";
        if (env.in_source_tree) {
          return std::format("build it with: python cli.py -b{}", config_flag);
        }
        return std::format("reinstall the SDK from a source tree with: python cli.py install{}", config_flag);
      }

      tool_result resolve_project_file(const filepath& target, filepath& resolved) {
        PROFILE_SECTION("resolve_project_file");
        std::error_code ec;
        const filepath absolute_target = std::filesystem::absolute(target, ec);
        if (ec || !std::filesystem::exists(absolute_target)) {
          return tool_result::error(std::format("'{}' does not exist", target.string()));
        }

        if (std::filesystem::is_regular_file(absolute_target)) {
          if (!is_project_file(absolute_target)) {
            return tool_result::error(std::format("'{}' is not a project file (expected a toml file with a [project] table)", absolute_target.string()));
          }
          resolved = absolute_target;
          return tool_result::ok();
        }

        if (!std::filesystem::is_directory(absolute_target)) {
          return tool_result::error(std::format("'{}' is neither a file nor a directory", absolute_target.string()));
        }

        std::vector<filepath> candidates;
        try {
          for (const auto& entry : std::filesystem::directory_iterator(absolute_target)) {
            if (entry.is_regular_file() && is_project_file(entry.path())) {
              candidates.push_back(entry.path());
            }
          }
        } catch (const std::exception& e) {
          return tool_result::error(std::format("failed to scan '{}': {}", absolute_target.string(), e.what()));
        }

        if (candidates.empty()) {
          return tool_result::error(std::format("no project file found in '{}'", absolute_target.string()));
        }
        if (candidates.size() > 1) {
          std::string listing = "";
          for (const filepath& candidate : candidates) {
            listing += std::format("\n  {}", candidate.string());
          }
          return tool_result::error(std::format("multiple project files found in '{}', pass one explicitly:{}", absolute_target.string(), listing));
        }

        resolved = candidates[0];
        return tool_result::ok();
      }

      tool_result resolve_editor(const environment_paths& env, const opt<std::string>& req_config, editor_location& out_location) {
        PROFILE_SECTION("resolve_editor");
        if (req_config.has_value()) {
          const std::string& config = req_config.value();
          if (!is_valid_build_config(config)) {
            return tool_result::error(std::format("invalid build config '{}' (expected Debug, Release, Profile, or ProfileD)", config));
          }

          const filepath candidate = env.editor_executable(config);
          if (!std::filesystem::exists(candidate)) {
            return tool_result::error(std::format("no {} editor build at '{}' ({})", config, candidate.string(), editor_missing_hint(env, config)));
          }

          out_location = { .executable = candidate, .label = candidate.parent_path().filename().string() };
          return tool_result::ok();
        }

        for (const std::string_view config : build_config_probe_order()) {
          const filepath candidate = env.editor_executable(config);
          if (std::filesystem::exists(candidate)) {
            out_location = { .executable = candidate, .label = candidate.parent_path().filename().string() };
            return tool_result::ok();
          }
        }

        return tool_result::error(std::format("no editor build found under '{}' ({})",
                                              editor_search_root(env).string(), editor_missing_hint(env, std::nullopt)));
      }

    }  // namespace

    std::string_view open_project_tool::usage() const {
      return kOpenUsage;
    }

    tool_result open_project_tool::execute(tool_context& ctx, std::span<const std::string> args) {
      PROFILE_SECTION("open_project_tool::execute");
      opt<filepath> target = std::nullopt;
      opt<std::string> requested_config = std::nullopt;
      bool wait_for_exit = false;
      bool dry_run = false;

      for (size_t i = 0; i < args.size(); ++i) {
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
          requested_config = value.value();
        } else if (arg == "--wait") {
          wait_for_exit = true;
        } else if (arg == "--dry-run") {
          dry_run = true;
        } else if (arg == "--env-root") {
          opt<std::string> value = flag_value();
          if (!value.has_value()) {
            return tool_result::error(std::format("'{}' requires a path value", arg));
          }
          ctx.env = locate_environment(filepath(value.value()));
          if (!ctx.env.found) {
            return tool_result::error(std::format("'{}' is not an Other Environment root", value.value()));
          }
        } else if (arg.starts_with("-")) {
          return tool_result::error(std::format("unknown option '{}'\n{}", arg, kOpenUsage));
        } else if (!target.has_value()) {
          target = filepath(arg);
        } else {
          return tool_result::error(std::format("unexpected argument '{}'\n{}", arg, kOpenUsage));
        }
      }

      filepath search_target = target.value_or(ctx.working_directory);
      if (search_target.is_relative()) {
        search_target = ctx.working_directory / search_target;
      }

      filepath project_file = "";
      if (tool_result resolved = resolve_project_file(search_target, project_file); !resolved.success()) {
        return resolved;
      }
      if (!ctx.env.found) {
        return tool_result::error("no Other Environment found (set OTHER_ENVIRONMENT_ROOT, pass --env-root, or run from source tree)");
      }

      editor_location editor;
      if (tool_result res = resolve_editor(ctx.env, requested_config, editor); !res.success()) {
        return res;
      }

      if (!std::filesystem::exists(ctx.env.editor_config)) {
        return tool_result::error(std::format("editor config '{}' does not exist{}", ctx.env.editor_config.string(),
                                              ctx.env.in_source_tree ? "" : " (reinstall the SDK; the editor and its config ship together)"));
      }

      /// the editor resolves engine resources relative to its working directory, so it
      ///  always launches from the environment root; the project rides along via -f
      const process_launch launch = {
        .executable = editor.executable,
        .arguments = { ctx.env.editor_config.string(), "-f", project_file.string() },
        .working_directory = ctx.env.root,
        .wait_for_exit = wait_for_exit,
        .new_console = !wait_for_exit,
      };

      if (dry_run) {
        ctx.print("would launch: {}", format_command_line(launch));
        ctx.print("     from cwd: {}", launch.working_directory.string());
        return tool_result::ok("dry run only, nothing launched");
      }

      const process_result launched = launch_process(launch);
      if (!launched.started) {
        return tool_result::error(std::format("failed to launch the editor: {}", launched.error));
      }

      if (wait_for_exit) {
        return { .code = launched.exit_code,
                 .message = std::format("editor exited with code {}", launched.exit_code) };
      }
      return tool_result::ok(std::format("launched other_editor [{}] with '{}'", editor.label, project_file.string()));
    }

  }  // namespace cli
}  // namespace other
