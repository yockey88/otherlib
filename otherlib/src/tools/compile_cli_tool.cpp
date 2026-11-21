/**
 * \file tools/compile_cli_tool.cpp
 **/
#include "tools/compile_cli_tool.hpp"

#include "core/profiler.hpp"

#include "project/project_compiler.hpp"
#include "tools/scene_cli_tool.hpp"
#include "tools/vm_cli_tool.hpp"
#include "vm/command_bus.hpp"

namespace other {
  namespace cli {
    namespace {

      constexpr std::string_view kUsage =
        "usage: oecli compile <project> [options]\n"
        "\n"
        "<project> is a project file or the folder that holds it. the package is a folder named\n"
        "after the game, made inside the output folder: the runtime, the project's boot program,\n"
        "its script assembly, and what it needs of the engine.\n"
        "\n"
        "options:\n"
        "  -o, --out <dir>            where the package folder is made (default: 'packages' in the working directory)\n"
        "  -c, --config <config>      Release (default) or Debug; a Debug package gets '-debug' on its name\n"
        "      --public-name <name>   the game's name for this package: its folder, executable and window title\n"
        "      --starting-scene <s>   the scene the game starts in, when the project names none or another is wanted\n"
        "      --main-pipeline <p>    the pipeline whose image reaches the window, when the project has several\n"
        "\n"
        "the three names override the project file for this package only; nothing is written back to it";

      struct parsed_arguments {
        opt<filepath> project = std::nullopt;
        compile_settings settings = {};
        opt<std::string> error = std::nullopt;
      };

      parsed_arguments parse_arguments(std::span<const std::string> args) {
        parsed_arguments parsed = {};
        for (size_t i = 0; i < args.size(); ++i) {
          const std::string& arg = args[i];
          const auto value = [&](opt<std::string>& into) -> bool {
            if (i + 1 >= args.size()) {
              parsed.error = std::format("'{}' expects a value", arg);
              return false;
            }
            into = args[++i];
            return true;
          };

          opt<std::string> text;
          if (arg == "-o" || arg == "--out") {
            if (!value(text)) {
              return parsed;
            }
            parsed.settings.output_directory = filepath(*text);
          } else if (arg == "-c" || arg == "--config") {
            if (!value(text)) {
              return parsed;
            }
            if (*text != "Debug" && *text != "Release") {
              parsed.error = std::format("a package is Debug or Release, not '{}'", *text);
              return parsed;
            }
            parsed.settings.config = *text == "Debug" ? package_config::DEBUG : package_config::RELEASE;
          } else if (arg == "--public-name") {
            if (!value(parsed.settings.public_name)) {
              return parsed;
            }
          } else if (arg == "--starting-scene") {
            if (!value(parsed.settings.starting_scene)) {
              return parsed;
            }
          } else if (arg == "--main-pipeline") {
            if (!value(parsed.settings.main_pipeline)) {
              return parsed;
            }
          } else if (!arg.empty() && arg.front() == '-') {
            parsed.error = std::format("unknown option '{}'", arg);
            return parsed;
          } else if (parsed.project.has_value()) {
            parsed.error = std::format("one project at a time: '{}' and '{}' were both given", parsed.project->string(), arg);
            return parsed;
          } else {
            parsed.project = filepath(arg);
          }
        }

        if (!parsed.project.has_value()) {
          parsed.error = "missing project";
        }
        return parsed;
      }

    }  // namespace

    std::string_view compile_tool::usage() const {
      return kUsage;
    }

    tool_result compile_tool::execute(tool_context& ctx, std::span<const std::string> args) {
      PROFILE_SECTION("compile_tool::execute");
      ensure_cli_runtime();

      parsed_arguments parsed = parse_arguments(args);
      if (parsed.error.has_value()) {
        return tool_result::error(std::format("{}\n{}", *parsed.error, kUsage));
      }

      const auto from_working_directory = [&ctx](const filepath& path) -> filepath {
        return path.is_absolute() ? path : ctx.working_directory / path;
      };
      parsed.settings.project_file = from_working_directory(*parsed.project);
      parsed.settings.output_directory = from_working_directory(parsed.settings.output_directory.empty() ? filepath{ "packages" } : parsed.settings.output_directory);

      command_bus local_bus;
      /// warnings are logged as they are found, which is where this tool's caller reads them
      const compile_report report = compile_project(tool_command_bus(local_bus), parsed.settings);
      if (!report.success) {
        std::string reasons;
        for (const std::string& error : report.errors) {
          reasons += std::format("\n  {}", error);
        }
        return tool_result::error(std::format("'{}' was not packaged:{}", parsed.project->string(), reasons));
      }
      return tool_result::ok(std::format("packaged '{}' as '{}'", parsed.project->string(), report.package_root.string()));
    }

  }  // namespace cli
}  // namespace other
