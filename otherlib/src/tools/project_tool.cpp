/**
 * \file tools/project_tool.cpp
 **/
#include "tools/project_tool.hpp"

#include "dotnet/native_string.hpp"

namespace other {

  void project_tool::generate_dotnet_project(const filepath& csproj_path) {
    OTHER_ASSERT(!csproj_path.empty(), "Csproj path is empty in generate_dotnet_project.");
    OTHER_ASSERT(!std::filesystem::exists(csproj_path), "Csproj file '{}' already exists in generate_dotnet_project.", csproj_path.string());

    native_string path = native_string::new_str(csproj_path.string());
    invoke_tool_method("CreateDefaultCsproj", path);
    native_string::free_str(path);

    dotnet_project_path = csproj_path;
  }

  void project_tool::start_project_build(const filepath& csproj_path) {
    OTHER_ASSERT(!csproj_path.empty(), "Dotnet project path is empty in start_project_build.");
    OTHER_ASSERT(std::filesystem::exists(csproj_path), "Dotnet project file '{}' does not exist.", csproj_path.string());

    native_string path = native_string::new_str(csproj_path.string());
    invoke_tool_method("SetCsprojFilePath", path);
    invoke_tool_method("StartCsprojBuild");
    native_string::free_str(path);

    dotnet_project_path = csproj_path;

    collect_cs_script_files(csproj_path.parent_path());
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

  void project_tool::collect_cs_script_files(const filepath& directory) {
    OTHER_ASSERT(std::filesystem::exists(directory), "Directory '{}' does not exist in collect_cs_script_files.", directory.string());
    OTHER_ASSERT(std::filesystem::is_directory(directory), "Path '{}' is not a directory in collect_cs_script_files.", directory.string());

    cs_script_files.clear();
    for (auto itr = std::filesystem::recursive_directory_iterator(directory); itr != std::filesystem::recursive_directory_iterator(); ++itr) {
      auto& entry = *itr;
      if (entry.is_directory() && (entry.path().filename() == "bin" || entry.path().filename() == "obj")) {
        itr.disable_recursion_pending();
        continue;
      }
      if (entry.is_regular_file() && entry.path().extension() == ".cs") {
        cs_script_files.push_back(entry.path());
      }
    }

    CORE_LOG_DEBUG("Collected {} C# script files from project directory '{}'", cs_script_files.size(), directory.string());
  }

}  // namespace other