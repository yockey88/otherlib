/**
 * \file cli/tools/create_project.cpp
 **/
#include "cli/tools/create_project.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <utility>
#include <vector>

#include "core/profiler.hpp"

namespace other {
  namespace cli {
    namespace {

      constexpr std::string_view kCreateUsage =
        R"(usage: create <name> [options...]

  Creates a new project directory <name> containing the project file, a project
  runtime script, a starter scene, and (when an environment is found) a C# script
  project referencing the environment's assemblies.

  <name> must start with a letter and may contain letters, digits, '-' and '_'.

  options:
    -d, --dir <path>        parent directory for the project (default: working directory)
    -a, --author <name>     author written into the project metadata (default: N/A)
        --description <d>   description written into the project metadata (default: N/A)
        --env-root <path>   explicit Other Environment root to wire the C# project against)";

      constexpr std::string_view kProjectFileTemplate =
        R"__([project]
metadata = [
  { key = "name", value = "${project-name}" },
  { key = "description", value = "${project-description}" },
  { key = "author", value = "${project-author}" },
  { key = "version", value = "0.1.0" }
]

[filesystem]
working-directory = "${project-name}"

[scripting]
projectrc-path = "${project-name}/${project-name}.lua"
${cs-project-line}
[scene-graph]
starting-scene = "main"
scenes = [
  { name = "main", path = "${project-name}/assets/scenes/main.lua" }
]
graph = [
  { name = "main", incoming = [], outgoing = [] }
]
)__";

      constexpr std::string_view kCsProjectLine = "cs_project = \"${project-name}/${csproj-name}.csproj\"\n";

      constexpr std::string_view kProjectRcTemplate =
        R"__(--- ${project-name} runtime configuration; runs when the environment loads the project
print("[${project-name}] project loaded")
)__";

      constexpr std::string_view kStarterSceneTemplate =
        R"__(--- default scene for ${project-name}
local camera_pos = Vec3:new(0.0, 2.0, 6.0)
local camera = Other:SceneObject():new("Camera", camera_pos)
local cam = camera:AttachCamera()
cam.sensitivity = 10.0
cam:Look(camera_pos, Vec3:new(0.0, 0.0, 0.0))
camera:AddTag("main-camera")

return {
  Objects = { camera }
}
)__";

      constexpr std::string_view kCsprojTemplate =
        R"__(<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup>
    <OutputType>Library</OutputType>
    <TargetFramework>net9.0</TargetFramework>
    <RootNamespace></RootNamespace>
    <AppendTargetFrameworkToOutputPath>false</AppendTargetFrameworkToOutputPath>
  </PropertyGroup>
  <ItemGroup Condition="'$(Configuration)' == 'Debug'">
    <Reference Include="OtherCs">
      <HintPath>${othercs-debug}</HintPath>
    </Reference>
  </ItemGroup>
  <ItemGroup Condition="'$(Configuration)' == 'Release'">
    <Reference Include="OtherCs">
      <HintPath>${othercs-release}</HintPath>
    </Reference>
  </ItemGroup>
</Project>
)__";

      using template_variables = std::vector<std::pair<std::string_view, std::string>>;

      /// replaces every ${key} occurrence, in variable order, so a variable's value may
      ///  itself contain later variables (kCsProjectLine relies on this)
      std::string apply_template(std::string_view template_text, const template_variables& variables) {
        std::string result{ template_text };
        for (const auto& [key, value] : variables) {
          const std::string tag = std::format("${{{}}}", key);
          size_t pos = 0;
          while ((pos = result.find(tag, pos)) != std::string::npos) {
            result.replace(pos, tag.size(), value);
            pos += value.size();
          }
        }
        return result;
      }

      bool is_valid_project_name(std::string_view name) {
        if (name.empty() || std::isalpha(static_cast<unsigned char>(name[0])) == 0) {
          return false;
        }
        return std::ranges::all_of(name, [](char c) {
          return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '-' || c == '_';
        });
      }

      /// "my-game" -> "MyGame", matching the TestProject/SpaceSim csproj naming
      std::string pascal_case(std::string_view name) {
        std::string result = "";
        bool upper_next = true;
        for (const char c : name) {
          if (c == '-' || c == '_') {
            upper_next = true;
            continue;
          }
          result.push_back(upper_next ? static_cast<char>(std::toupper(static_cast<unsigned char>(c))) : c);
          upper_next = false;
        }
        return result;
      }

      std::string escape_toml_string(std::string_view text) {
        std::string escaped = "";
        for (const char c : text) {
          if (c == '\\' || c == '"') {
            escaped.push_back('\\');
          }
          escaped.push_back(c);
        }
        return escaped;
      }

      bool write_text_file(const filepath& path, std::string_view contents, std::string& error) {
        /// binary mode keeps the generated files LF-only
        std::ofstream file(path, std::ios::binary);
        if (!file.is_open()) {
          error = std::format("failed to open '{}' for writing", path.string());
          return false;
        }
        file.write(contents.data(), static_cast<std::streamsize>(contents.size()));
        if (!file.good()) {
          error = std::format("failed while writing '{}'", path.string());
          return false;
        }
        return true;
      }

    }  // namespace

    std::string_view create_project_tool::usage() const {
      return kCreateUsage;
    }

    tool_result create_project_tool::execute(tool_context& ctx, std::span<const std::string> args) {
      PROFILE_SECTION("create_project_tool::execute");
      std::string project_name = "";
      opt<filepath> destination = std::nullopt;
      std::string author = "N/A";
      std::string description = "N/A";

      for (size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];
        auto flag_value = [&args, &i]() -> opt<std::string> {
          if (i + 1 >= args.size()) {
            return std::nullopt;
          }
          return args[++i];
        };

        if (arg == "--dir" || arg == "-d") {
          opt<std::string> value = flag_value();
          if (!value.has_value()) {
            return tool_result::error(std::format("'{}' requires a path value", arg));
          }
          destination = filepath(value.value());
        } else if (arg == "--author" || arg == "-a") {
          opt<std::string> value = flag_value();
          if (!value.has_value()) {
            return tool_result::error(std::format("'{}' requires a value", arg));
          }
          author = value.value();
        } else if (arg == "--description") {
          opt<std::string> value = flag_value();
          if (!value.has_value()) {
            return tool_result::error(std::format("'{}' requires a value", arg));
          }
          description = value.value();
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
          return tool_result::error(std::format("unknown option '{}'\n{}", arg, kCreateUsage));
        } else if (project_name.empty()) {
          project_name = arg;
        } else {
          return tool_result::error(std::format("unexpected argument '{}'\n{}", arg, kCreateUsage));
        }
      }

      if (project_name.empty()) {
        return tool_result::error(std::format("no project name given\n{}", kCreateUsage));
      }
      if (!is_valid_project_name(project_name)) {
        return tool_result::error(
          std::format("invalid project name '{}' (must start with a letter; letters, digits, '-', '_' only)", project_name));
      }

      filepath base_dir = destination.value_or(ctx.working_directory);
      if (base_dir.is_relative()) {
        base_dir = ctx.working_directory / base_dir;
      }
      base_dir = base_dir.lexically_normal();

      const filepath project_dir = base_dir / project_name;
      if (std::filesystem::exists(project_dir)) {
        return tool_result::error(std::format("'{}' already exists", project_dir.string()));
      }

      const bool with_cs_project = ctx.env.found;
      const std::string csproj_name = pascal_case(project_name);

      template_variables variables = {
        { "cs-project-line", with_cs_project ? std::string{ kCsProjectLine } : std::string{} },
        { "project-name", project_name },
        { "project-description", escape_toml_string(description) },
        { "project-author", escape_toml_string(author) },
        { "csproj-name", csproj_name },
        { "othercs-debug", ctx.env.othercs_assembly("Debug").string() },
        { "othercs-release", ctx.env.othercs_assembly("Release").string() },
      };

      struct generated_file {
        filepath path;
        std::string contents;
      };
      std::vector<generated_file> files = {
        { project_dir / std::format("{}.toml", project_name), apply_template(kProjectFileTemplate, variables) },
        { project_dir / std::format("{}.lua", project_name), apply_template(kProjectRcTemplate, variables) },
        { project_dir / "assets" / "scenes" / "main.lua", apply_template(kStarterSceneTemplate, variables) },
      };
      if (with_cs_project) {
        files.push_back({ project_dir / std::format("{}.csproj", csproj_name), apply_template(kCsprojTemplate, variables) });
      }

      /// the project directory did not exist before this call, so on any failure the
      ///  half-created tree is removed wholesale
      auto fail_and_cleanup = [&project_dir](std::string error) {
        std::error_code ec;
        std::filesystem::remove_all(project_dir, ec);
        return tool_result::error(std::move(error));
      };

      std::error_code ec;
      for (const filepath& dir : { project_dir, project_dir / "assets" / "scenes", project_dir / "src" }) {
        std::filesystem::create_directories(dir, ec);
        if (ec) {
          return fail_and_cleanup(std::format("failed to create directory '{}': {}", dir.string(), ec.message()));
        }
      }

      for (const generated_file& file : files) {
        std::string error = "";
        if (!write_text_file(file.path, file.contents, error)) {
          return fail_and_cleanup(std::move(error));
        }
        ctx.print("  created {}", file.path.string());
      }

      if (!with_cs_project) {
        ctx.print("  note: no Other Environment found, skipped the C# script project");
        ctx.print("        (set OTHER_ENVIRONMENT_ROOT or pass --env-root, then re-create to wire scripts)");
      }

      const filepath project_file = files[0].path;
      return tool_result::ok(std::format("created project '{}'\nopen it with: oecli open \"{}\"", project_name, project_file.string()));
    }

  }  // namespace cli
}  // namespace other
