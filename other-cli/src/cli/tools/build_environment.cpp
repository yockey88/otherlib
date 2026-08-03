/**
 * \file cli/tools/build_environment.cpp
 **/
#include "cli/tools/build_environment.hpp"

#include <array>
#include <filesystem>
#include <vector>

#include "cli/tools/dev_common.hpp"

namespace other {
  namespace cli {
    namespace {

      constexpr std::string_view kBuildUsage =
        R"(usage: build [options...]

  Builds the Other Environment from source and stages the vendored runtime DLLs
  next to every built application. Must run against a source tree.

  options:
    -c, --config <config>   build config: Debug, Release, Profile, ProfileD
                            (default: Debug)
        --tests             configure with BUILD_OTHER_TESTS=ON so the test suites build
        --regen             rerun cmake project generation before building
        --target <name>     build a single cmake target instead of the full environment
        --env-root <path>   explicit source tree root to build
        --dry-run           print the commands instead of running them)";

      std::vector<filepath> runtime_dlls(const filepath& root, bool debug_family) {
        const std::string family = debug_family ? "Debug" : "Release";
        const std::string family_lower = debug_family ? "debug" : "release";

        const filepath extern_dir = root / "extern";
        std::vector<filepath> dlls = {
          extern_dir / "sdl" / "lib" / family_lower / "SDL3.dll",
          extern_dir / "assimp" / "lib" / "assimp-vc143-mt.dll",
          extern_dir / "python312" / "python312.dll",
          extern_dir / "sol2" / "lib" / "lua-5.4.4.dll",
          extern_dir / "jolt" / "bin" / family / "Jolt.dll",
          extern_dir / "physx" / "bin" / family / "PhysX_64.dll",
          extern_dir / "physx" / "bin" / family / "PhysXCommon_64.dll",
          extern_dir / "physx" / "bin" / family / "PhysXCooking_64.dll",
          extern_dir / "physx" / "bin" / family / "PhysXFoundation_64.dll",
          extern_dir / "physx" / "bin" / family / "PhysXGpu_64.dll",
        };
        if (debug_family) {
          dlls.push_back(extern_dir / "physx" / "bin" / family / "PVDRuntime_64.dll");
        }
        return dlls;
      }

      void stage_runtime_dlls(const tool_context& ctx, const filepath& root, std::string_view config, bool dry_run) {
        const bool debug_family = (config == "Debug" || config == "ProfileD");

        /// every directory that holds a runnable build output; plugin outputs (spacesim,
        ///  test-project, ...) load inside the editor process and need no copies
        const std::array<filepath, 7> output_dirs = {
          root / "build" / "other-editor",
          root / "build" / "other-server",
          root / "build" / "other-cli",
          root / "build" / "other-cli" / "user",
          root / "build" / "scratch",
          root / "build" / "tests",
          root / "build" / "tests" / "harness",
        };

        const std::vector<filepath> dlls = runtime_dlls(root, debug_family);
        if (dry_run) {
          ctx.print("would stage {} runtime DLLs into the built application directories", dlls.size());
          return;
        }

        size_t copied = 0;
        size_t up_to_date = 0;
        for (const filepath& dll : dlls) {
          if (!std::filesystem::exists(dll)) {
            ctx.print("warning: runtime dll '{}' does not exist", dll.string());
            continue;
          }
          for (const filepath& output_dir : output_dirs) {
            const filepath destination = output_dir / config;
            if (!std::filesystem::exists(destination)) {
              continue;
            }

            /// the vendored dlls never change in place, so a same-sized copy is current;
            ///  this also keeps the pass from rewriting dlls a running process (often
            ///  oecli itself) holds loaded
            const filepath staged = destination / dll.filename();
            std::error_code ec;
            if (std::filesystem::exists(staged) && std::filesystem::file_size(staged, ec) == std::filesystem::file_size(dll, ec)) {
              ++up_to_date;
              continue;
            }

            std::filesystem::copy_file(dll, staged, std::filesystem::copy_options::overwrite_existing, ec);
            if (ec) {
              ctx.print("warning: failed to stage '{}' into '{}': {}", dll.filename().string(), destination.string(), ec.message());
              continue;
            }
            ++copied;
          }
        }
        ctx.print("staged {} runtime DLL copies ({} already up to date) [{}]", copied, up_to_date, debug_family ? "Debug" : "Release");
      }

    }  // namespace

    std::string_view build_environment_tool::usage() const {
      return kBuildUsage;
    }

    tool_result build_environment_tool::execute(tool_context& ctx, std::span<const std::string> args) {
      dev_tool_options options;
      bool with_tests = false;
      bool regen = false;
      opt<std::string> target = std::nullopt;

      for (size_t i = 0; i < args.size(); ++i) {
        if (opt<tool_result> shared = try_parse_dev_flag(ctx, args, i, options); shared.has_value()) {
          if (!shared.value().success()) {
            return shared.value();
          }
          continue;
        }

        const std::string& arg = args[i];
        if (arg == "--tests") {
          with_tests = true;
        } else if (arg == "--regen") {
          regen = true;
        } else if (arg == "--target") {
          if (i + 1 >= args.size()) {
            return tool_result::error("'--target' requires a cmake target name");
          }
          target = args[++i];
        } else {
          return tool_result::error(std::format("unknown argument '{}'\n{}", arg, kBuildUsage));
        }
      }

      if (tool_result guard = require_source_tree(ctx, "build"); !guard.success()) {
        return guard;
      }

      const opt<filepath> cmake = find_program_on_path("cmake");
      if (!cmake.has_value()) {
        return tool_result::error("cmake not found on PATH (a CMake install is required to build the environment)");
      }

      const std::string config = options.config.value_or("Debug");
      const filepath root = ctx.env.root;
      const filepath build_dir = root / "build";

      /// --tests always reconfigures so BUILD_OTHER_TESTS lands in the cache even when
      ///  the project files already exist
      const bool have_project_files = std::filesystem::exists(build_dir / "other.sln") || std::filesystem::exists(build_dir / "other.slnx");
      if (regen || with_tests || !have_project_files) {
        std::vector<std::string> configure_args = { "-S", root.string(), "-B", build_dir.string() };
        if (with_tests) {
          configure_args.push_back("-DBUILD_OTHER_TESTS=ON");
        }

        const tool_result configured = run_attached(ctx,
          { .executable = cmake.value(), .arguments = configure_args, .working_directory = root }, options.dry_run);
        if (!configured.success()) {
          return tool_result::error(std::format("cmake project generation failed (exit {})", configured.code));
        }
      }

      std::vector<std::string> build_args = { "--build", build_dir.string(), "--config", config, "--parallel" };
      if (target.has_value()) {
        build_args.push_back("--target");
        build_args.push_back(target.value());
      }

      sidestep_running_executable(ctx, build_dir, "build", options.dry_run);

      const tool_result built = run_attached(ctx,
        { .executable = cmake.value(), .arguments = build_args, .working_directory = root }, options.dry_run);
      if (!built.success()) {
        return { .code = built.code, .message = std::format("build failed (exit {})", built.code) };
      }

      stage_runtime_dlls(ctx, root, config, options.dry_run);
      if (options.dry_run) {
        return tool_result::ok("dry run only, nothing built");
      }
      return tool_result::ok(std::format("built Other Environment [{}]{}", config,
        target.has_value() ? std::format(" (target {})", target.value()) : ""));
    }

  }  // namespace cli
}  // namespace other
