/**
 * \file cli/tools/install_environment.cpp
 **/
#include "cli/tools/install_environment.hpp"

#include <filesystem>
#include <vector>

#include "cli/tools/dev_common.hpp"

namespace other {
  namespace cli {
    namespace {

      constexpr std::string_view kInstallUsage =
        R"(usage: install [options...]

  Installs the built Other Environment SDK with cmake --install. The prefix
  defaults to the one the build was configured with (C:/OtherEnvironment).

  options:
    -c, --config <config>   build config to install (default: Release)
        --prefix <path>     install into an explicit prefix instead
        --env-root <path>   explicit source tree root
        --dry-run           print the command instead of running it)";

      constexpr std::string_view kPackageUsage =
        R"(usage: package [options...]

  Packages the built environment into distributable artifacts with cpack, written
  to build/packages/. ZIP needs nothing extra; the NSIS generator produces the
  Windows installer and requires NSIS (makensis) on the machine.

  options:
    -c, --config <config>      build config to package (default: Release)
    -G, --generators <list>    semicolon-separated cpack generators
                               (default: ZIP; use "NSIS;ZIP" for the installer too)
        --env-root <path>      explicit source tree root
        --dry-run              print the command instead of running it)";

      /// both tools only make sense against a configured build tree
      tool_result require_configured_build(const filepath& build_dir) {
        if (!std::filesystem::exists(build_dir / "CMakeCache.txt")) {
          return tool_result::error(std::format("'{}' has not been configured yet (run: build)", build_dir.string()));
        }
        return tool_result::ok();
      }

    }  // namespace

    std::string_view install_environment_tool::usage() const {
      return kInstallUsage;
    }

    tool_result install_environment_tool::execute(tool_context& ctx, std::span<const std::string> args) {
      dev_tool_options options;
      opt<std::string> prefix = std::nullopt;

      for (size_t i = 0; i < args.size(); ++i) {
        if (opt<tool_result> shared = try_parse_dev_flag(ctx, args, i, options); shared.has_value()) {
          if (!shared.value().success()) {
            return shared.value();
          }
          continue;
        }

        const std::string& arg = args[i];
        if (arg == "--prefix") {
          if (i + 1 >= args.size()) {
            return tool_result::error("'--prefix' requires a path value");
          }
          prefix = args[++i];
        } else {
          return tool_result::error(std::format("unknown argument '{}'\n{}", arg, kInstallUsage));
        }
      }

      if (tool_result guard = require_source_tree(ctx, "install"); !guard.success()) {
        return guard;
      }

      const filepath build_dir = ctx.env.root / "build";
      if (tool_result configured = require_configured_build(build_dir); !configured.success()) {
        return configured;
      }

      const opt<filepath> cmake = find_program_on_path("cmake");
      if (!cmake.has_value()) {
        return tool_result::error("cmake not found on PATH (a CMake install is required to install the environment)");
      }

      const std::string config = options.config.value_or("Release");
      std::vector<std::string> install_args = { "--install", build_dir.string(), "--config", config };
      if (prefix.has_value()) {
        install_args.push_back("--prefix");
        install_args.push_back(prefix.value());
      }

      const tool_result installed = run_attached(ctx,
        { .executable = cmake.value(), .arguments = install_args, .working_directory = ctx.env.root }, options.dry_run);
      if (options.dry_run) {
        return tool_result::ok("dry run only, nothing installed");
      }
      if (!installed.success()) {
        return { .code = installed.code, .message = std::format("install failed (exit {})", installed.code) };
      }
      return tool_result::ok(std::format("installed Other Environment [{}]{}", config,
        prefix.has_value() ? std::format(" to '{}'", prefix.value()) : ""));
    }

    std::string_view package_environment_tool::usage() const {
      return kPackageUsage;
    }

    tool_result package_environment_tool::execute(tool_context& ctx, std::span<const std::string> args) {
      dev_tool_options options;
      std::string generators = "ZIP";

      for (size_t i = 0; i < args.size(); ++i) {
        if (opt<tool_result> shared = try_parse_dev_flag(ctx, args, i, options); shared.has_value()) {
          if (!shared.value().success()) {
            return shared.value();
          }
          continue;
        }

        const std::string& arg = args[i];
        if (arg == "--generators" || arg == "-G") {
          if (i + 1 >= args.size()) {
            return tool_result::error(std::format("'{}' requires a generator list (e.g. \"NSIS;ZIP\")", arg));
          }
          generators = args[++i];
        } else {
          return tool_result::error(std::format("unknown argument '{}'\n{}", arg, kPackageUsage));
        }
      }

      if (tool_result guard = require_source_tree(ctx, "package"); !guard.success()) {
        return guard;
      }

      const filepath build_dir = ctx.env.root / "build";
      if (tool_result configured = require_configured_build(build_dir); !configured.success()) {
        return configured;
      }

      const opt<filepath> cpack = find_program_on_path("cpack");
      if (!cpack.has_value()) {
        return tool_result::error("cpack not found on PATH (it ships with CMake)");
      }

      const std::string config = options.config.value_or("Release");
      const filepath packages_dir = build_dir / "packages";
      const std::vector<std::string> package_args = { "-G", generators, "-C", config, "-B", packages_dir.string() };

      const tool_result packaged = run_attached(ctx,
        { .executable = cpack.value(), .arguments = package_args, .working_directory = build_dir }, options.dry_run);
      if (options.dry_run) {
        return tool_result::ok("dry run only, nothing packaged");
      }
      if (!packaged.success()) {
        return { .code = packaged.code, .message = std::format("packaging failed (exit {})", packaged.code) };
      }
      return tool_result::ok(std::format("packages written to '{}'", packages_dir.string()));
    }

  }  // namespace cli
}  // namespace other
