/**
 * \file tools/project_tool.cpp
 **/
#include "tools/project_tool.hpp"

#include "core/profiler.hpp"
#include "dotnet/native_string.hpp"

namespace other {

  void project_tool::generate_dotnet_project(const filepath& csproj_path) {
    OTHER_ASSERT(!csproj_path.empty(), "Csproj path is empty in generate_dotnet_project.");
    OTHER_ASSERT(!std::filesystem::exists(csproj_path), "Csproj file '{}' already exists in generate_dotnet_project.", csproj_path.string());
    PROFILE_SECTION("project_tool::generate_dotnet_project");

    native_string path = native_string::new_str(csproj_path.string());
    invoke_tool_method("CreateDefaultCsproj", path);
    native_string::free_str(path);

    dotnet_project_path = csproj_path;
  }

  void project_tool::start_project_build(const filepath& csproj_path) {
    OTHER_ASSERT(!csproj_path.empty(), "Dotnet project path is empty in start_project_build.");
    OTHER_ASSERT(std::filesystem::exists(csproj_path), "Dotnet project file '{}' does not exist.", csproj_path.string());
    PROFILE_SECTION("project_tool::start_project_build");

    std::string build_config = get_project_build_config_string();
    native_string path = native_string::new_str(csproj_path.string());
    native_string config = native_string::new_str(build_config);

    invoke_tool_method("SetConfig", config);
    invoke_tool_method("SetCsprojFilePath", path);
    invoke_tool_method("StartCsprojBuild");

    native_string::free_str(config);
    native_string::free_str(path);

    dotnet_project_path = csproj_path;
  }

  bool project_tool::project_build_in_progress() const {
    return invoke_tool_method<nbool32>("IsBuildInProgress");
  }

  void project_tool::cleanup_build() {
    invoke_tool_method("CleanupBuild");
  }

  int32_t project_tool::get_build_result() const {
    return invoke_tool_method<int32_t>("GetBuildResult");
  }

}  // namespace other