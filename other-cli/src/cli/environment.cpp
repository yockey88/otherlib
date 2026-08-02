/**
 * \file cli/environment.cpp
 **/
#include "cli/environment.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <format>

#ifdef OTHER_ENVIRONMENT_WINDOWS
  #include <windows.h>
#endif

namespace other {
  namespace cli {
    namespace {

      constexpr std::string_view kDefaultInstallDir = "C:/OtherEnvironment";

#ifdef OTHER_ENVIRONMENT_WINDOWS
      constexpr std::string_view kExecutableSuffix = ".exe";
#else
      constexpr std::string_view kExecutableSuffix = "";
#endif

      bool is_source_tree_root(const filepath& path) {
        return std::filesystem::exists(path / "otherlib") &&
          std::filesystem::exists(path / "resources" / "editor-config.toml") &&
          std::filesystem::exists(path / "cmake" / "other_driver.cmake");
      }

      bool is_installed_root(const filepath& path) {
        return std::filesystem::exists(path / "cmake" / "otherlib-config.cmake");
      }

      bool is_environment_root(const filepath& path) {
        return is_source_tree_root(path) || is_installed_root(path);
      }

      opt<filepath> find_root_walking_up(const filepath& start) {
        std::error_code ec;
        filepath current = std::filesystem::absolute(start, ec);
        if (ec || current.empty()) {
          return std::nullopt;
        }

        while (true) {
          if (is_environment_root(current)) {
            return current;
          }

          filepath parent = current.parent_path();
          if (parent == current) {
            return std::nullopt;
          }
          current = parent;
        }
      }

      opt<filepath> current_executable_directory() {
#ifdef OTHER_ENVIRONMENT_WINDOWS
        std::array<wchar_t, 4096> buffer = {};
        DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0 || length >= buffer.size()) {
          return std::nullopt;
        }
        return filepath(std::wstring_view(buffer.data(), length)).parent_path();
#else
        return std::nullopt;
#endif
      }

      environment_paths make_environment(const filepath& root) {
        environment_paths env;
        env.root = root;
        env.found = true;
        env.in_source_tree = is_source_tree_root(root);

        if (env.in_source_tree) {
          env.editor_config = root / "resources" / "editor-config.toml";
          env.templates_dir = root / "templates";
        } else {
          /// the installed sdk does not ship the editor (or its config) yet
          env.editor_config = "";
          env.templates_dir = root / "project" / "templates";
        }

        return env;
      }

      /// the C# assembly output only splits Debug/Release (Profile maps to Release,
      ///  ProfileD to Debug), mirroring ASSEMBLY_CONFIG in the root CMakeLists
      std::string_view assembly_config_for(std::string_view build_config) {
        if (build_config == "Debug" || build_config == "ProfileD") {
          return "Debug";
        }
        return "Release";
      }

    }  // namespace

    filepath environment_paths::editor_executable(std::string_view build_config) const {
      if (!found || !in_source_tree) {
        return "";
      }
      return root / "build" / "other-editor" / build_config / std::format("other_editor{}", kExecutableSuffix);
    }

    filepath environment_paths::othercs_assembly(std::string_view build_config) const {
      if (!found) {
        return "";
      }
      if (in_source_tree) {
        return root / "build" / "other-csharp" / assembly_config_for(build_config) / "OtherCs.dll";
      }
      return root / "dotnet-assemblies" / "OtherCs.dll";
    }

    bool is_valid_build_config(std::string_view build_config) {
      return std::ranges::find(kBuildConfigs, build_config) != kBuildConfigs.end();
    }

    environment_paths locate_environment(const opt<filepath>& explicit_root) {
      /// an explicit root is taken at face value: no walking, no fallbacks
      if (explicit_root.has_value()) {
        std::error_code ec;
        filepath root = std::filesystem::absolute(explicit_root.value(), ec);
        if (!ec && is_environment_root(root)) {
          return make_environment(root);
        }
        return {};
      }

      if (const char* env_root = std::getenv("OTHER_ENVIRONMENT_ROOT"); env_root != nullptr && env_root[0] != '\0') {
        if (opt<filepath> root = find_root_walking_up(env_root); root.has_value()) {
          return make_environment(root.value());
        }
      }

      if (opt<filepath> exe_dir = current_executable_directory(); exe_dir.has_value()) {
        if (opt<filepath> root = find_root_walking_up(exe_dir.value()); root.has_value()) {
          return make_environment(root.value());
        }
      }

      std::error_code ec;
      filepath cwd = std::filesystem::current_path(ec);
      if (!ec) {
        if (opt<filepath> root = find_root_walking_up(cwd); root.has_value()) {
          return make_environment(root.value());
        }
      }

      if (filepath install_dir = filepath(kDefaultInstallDir); is_environment_root(install_dir)) {
        return make_environment(install_dir);
      }

      return {};
    }

    std::array<std::string_view, 4> build_config_probe_order() {
      static const std::array<std::string_view, 4> order = [] {
        std::array<std::string_view, 4> configs = kBuildConfigs;
        const std::string compiled = get_environment_build_config_string();
        auto it = std::ranges::find(configs, std::string_view(compiled));
        if (it != configs.end()) {
          std::rotate(configs.begin(), it, it + 1);
        }
        return configs;
      }();
      return order;
    }

  }  // namespace cli
}  // namespace other
