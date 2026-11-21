/**
 * \file project/project_compiler.hpp
 * turns a project into a folder a player can run: the runtime under the game's name, the
 * project's boot program, its script assembly, the pipelines it draws with, and only the
 * parts of the engine those need
 **/
#ifndef OTHERLIB_PROJECT_PROJECT_COMPILER_HPP
#define OTHERLIB_PROJECT_PROJECT_COMPILER_HPP

#include <string>
#include <string_view>

#include "core/defines.hpp"
#include "data-structures/std_container.hpp"

namespace other {

  class command_bus;

  enum class package_config : uint8_t {
    RELEASE,
    DEBUG,
  };

  /// where the pieces of the engine that go into a package are taken from
  struct engine_sources {
    /// other_runtime.exe; the DLLs beside it go with it
    filepath runtime_executable;
    /// the folder holding OtherCs.dll and OtherCsBindings.dll
    filepath assemblies;
    /// OtherCsBindings.runtimeconfig.json
    filepath dotnet_runtime_config;
    /// the folder of the engine's .lua files
    filepath lua;
    /// the engine root: resources/ is under it, and engine://x is root/x
    filepath root;
  };

  struct compile_settings {
    filepath project_file;
    /// the package folder is made inside this one
    filepath output_directory;
    /// each overrides the project's own value when set, and is not written back to it
    opt<std::string> public_name;
    opt<std::string> starting_scene;
    opt<std::string> main_pipeline;
    package_config config = package_config::RELEASE;
    /// empty means the engine this code is running from, as locate_engine_sources finds it
    opt<engine_sources> engine;
  };

  struct compile_report {
    bool success = false;
    /// the finished package; set only on success
    filepath package_root;
    ostd::vector<std::string> errors;
    ostd::vector<std::string> warnings;
  };

  /// marks a folder as one compile made, so compile will replace it and nothing else
  constexpr inline std::string_view kPackageMarker = ".other-package";

  /// a source tree has the runtime under build/, an installed SDK under bin/<family>. empty,
  ///  with the reason in errors, when the runtime of that family has not been built
  opt<engine_sources> locate_engine_sources(package_config config, ostd::vector<std::string>& errors);

  /// why name cannot be a game's public name; empty when it can. the name becomes the
  ///  package folder, the executable and the log file, so it has to be a file name
  opt<std::string> invalid_public_name_reason(std::string_view name);

  /// text with key = "value" in [table]: the line is replaced when it is there, added under
  ///  the table's header when it is not, and the table is added at the end when that is
  ///  missing. every other line, comments included, is left as it was
  std::string set_toml_string(std::string_view text, std::string_view table, std::string_view key, std::string_view value);
  /// text without key's line in [table]; unchanged when there is none
  std::string remove_toml_key(std::string_view text, std::string_view table, std::string_view key);

  /// builds the package in a staging folder and swaps it in when every stage has passed, so
  ///  a failed compile never replaces a good package. commands resolve against the devices
  ///  on bus, as in build_project_program
  compile_report compile_project(const command_bus& bus, const compile_settings& settings);

}  // namespace other

#endif  // OTHERLIB_PROJECT_PROJECT_COMPILER_HPP
