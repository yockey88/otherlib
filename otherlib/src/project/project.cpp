/**
 * \file project/project.cpp
 **/
#include "project/project.hpp"

#include <string_view>

#include <toml++/toml.hpp>

#include "file/filesystem.hpp"

#include "script/scripting_environment.hpp"

#include "driver/systems/job_driver_system.hpp"
#include "driver/systems/project_system.hpp"
#include "tools/project_tool.hpp"

namespace other {
  namespace detail {

    void process_metadata(const toml::table& table, project::metadata& metadata);
    task load_dotnet_project(project* p, filepath csproj_path);

  }  // namespace detail

  project::project(project_system* proj_system)
      : system(proj_system) {
    OTHER_ASSERT(system != nullptr, "Project system pointer is null in project constructor.");
  }

  void project::load_from_file(driver_kernel* kernel, const filepath& path) {
    OTHER_ASSERT(kernel != nullptr, "Driver kernel is null in process_project_file.");

    OTHER_ASSERT(std::filesystem::exists(path), "Project file '{}' does not exist.", path.string());
    OTHER_ASSERT(std::filesystem::is_regular_file(path), "Project file '{}' is not a regular file.", path.string());
    CORE_LOG_INFO("Loading project from file: '{}'", path.string());

    auto* fs = subsystem<file_system>::get();
    OTHER_ASSERT(fs != nullptr, "file_system subsystem is not available.");

    project_file_handle = fs->register_local_file(path.string());
    OTHER_ASSERT(project_file_handle != nullptr, "Failed to open project file '{}'", path.string());
    current_state = LOADING;

#ifdef OTHER_PROJECT_FILE_BINARY_FORMAT
    CORE_LOG_DEBUG("Loading TOML project file '{}'", project_file_handle->virtual_path());
    natural_t size = project_file_handle->size();
    file_buffer.allocate(size);

    const bool open = project_file_handle->open(file_mode::READ);
    uint64_t bytes_read = project_file_handle->read(file_buffer.view_bytes());
    project_file_handle->close();

    CORE_LOG_INFO("Successfully loaded project file '{}'", project_file_handle->virtual_path());
    CORE_LOG_DEBUG("Project file size: {} bytes", size);
    CORE_LOG_DEBUG("Data: {}", file_buffer.dump_buffer());
#elif defined(OTHER_PROJECT_FILE_TOML_FORMAT)
    CORE_LOG_DEBUG("Loading TOML project file '{}'", project_file_handle->virtual_path());
    std::string file_contents = project_file_handle->read_all_as_string();
    CORE_LOG_DEBUG("\n{}", file_contents);

    toml::table table;
    try {
      table = toml::parse(file_contents);
      detail::process_metadata(table, project_metadata);
      {
        std::stringstream ss;
        ss << project_metadata.name << " v" << project_metadata.version << " by " << project_metadata.author;
        if (!project_metadata.description.empty()) {
          ss << "\n"
             << " - " << project_metadata.description;
        }
        CORE_LOG_INFO("[PROJECT] {}", ss.str());
      }
    } catch (const std::exception& e) {
      CORE_LOG_ERROR("Failed to parse project file: {}", e.what());
    } catch (...) {
      CORE_LOG_ERROR("An unknown error occurred while parsing the project file.");
    }

    toml::node_view scripting_node = table.at_path("scripting");
    if (!scripting_node) {
      CORE_LOG_DEBUG("No 'scripting' section found in project file, skipping scripting data processing.");
      return;
    }
    if (!scripting_node.is_table()) {
      CORE_LOG_ERROR("Expected 'scripting' section to be a table.");
      return;
    }

    const auto& scripting_table = scripting_node.as_table();
    if (scripting_table == nullptr) {
      CORE_LOG_ERROR("'scripting' section is not a valid table.");
      return;
    }

    CORE_LOG_DEBUG("Processing project scripting data");

    bool waiting_for_script_load = false;
    scripting_table->for_each([this, kernel, &waiting_for_script_load](const toml::key& key, const toml::node& value) {
      std::string k{ key.str() };

      CORE_LOG_DEBUG("Processing scripting entry with key '{}'", k);
      if (k == "cs_project") {
        if (!value.is_string()) {
          CORE_LOG_ERROR("Expected 'cs_project' value to be a string representing the path to the .csproj file.");
        } else {
          waiting_for_script_load = true;
          system->sibling<job_driver_system>(*kernel).post_coroutine(detail::load_dotnet_project(this, value.as_string()->get()));
        }
      }
    });

    auto rc_path = scripting_table->get("projectrc-path");
    if (rc_path && rc_path->is_string()) {
      this->rc_path = rc_path->as_string()->get();
      CORE_LOG_DEBUG("Project runtime configuration script path set to '{}'", this->rc_path.string());
    } else {
      CORE_LOG_DEBUG("No 'projectrc-path' specified in project file. Driver environment runtime script will not be loaded.");
    }

    if (!waiting_for_script_load) {
      set_state(LOADED);
    }
#else
    static_assert(false, "No project file format defined. define OTHER_PROJECT_FILE_XXX_FORMAT macro.");
#endif
  }

  void project::unload() {
    if (project_file_handle != nullptr && project_file_handle->is_open()) {
      project_file_handle->close();
    }
    project_file_handle = nullptr;
    file_buffer.release();

    if (project_assembly != nullptr) {
      auto* env = subsystem<scripting_environment>::get();
      OTHER_ASSERT(env != nullptr, "Scripting environment subsystem is not available.");
      env->unload_dotnet_module(project_assembly);
    }
    project_assembly = nullptr;
  }

  void project::set_state(state new_state) {
    if (new_state == current_state) {
      return;
    }

    if (new_state == EMPTY) {
    } else if (new_state == LOADING) {
    } else if (new_state == LOADED) {
      system->get_driver().trigger_event("project.loaded");
    } else if (new_state == UNLOADING) {
    } else {
      CORE_LOG_WARN("Project state changed to unknown state {}", new_state);
    }
    current_state = new_state;
  }

  namespace detail {

    task load_dotnet_project(project* p, filepath csproj_path) {
      OTHER_ASSERT(p != nullptr, "Project pointer is null in load_dotnet_project.");
      CORE_LOG_INFO("Loading .NET project from'{}'", csproj_path.string());

      project_tool tool;

      /// create dotnet project for the loaded project
      if (!std::filesystem::exists(csproj_path)) {
        CORE_LOG_INFO("No .NET project file found at '{}', creating a new one.", csproj_path.string());
        tool.generate_dotnet_project(csproj_path);
      }

      /// .csproj file exists we go straight to building it
      tool.start_project_build(csproj_path);

      do {
        co_await task::yield();
      } while (tool.project_build_in_progress());

      int32_t result = tool.get_build_result();
      tool.cleanup_build();

      if (result != 0) {
        CORE_LOG_ERROR("Failed to build .NET project '{}', build process exited with code {}.", csproj_path.string(), result);
        co_return;
      }

      filepath filename = csproj_path.filename().replace_extension("dll");
      filepath directory = csproj_path.parent_path() / "bin" / "Debug";
      filepath dll_path = directory / filename;

      /// check dll exists and load it
      if (!std::filesystem::exists(dll_path)) {
        CORE_LOG_ERROR("Expected compiled .NET assembly '{}' does not exist after building the project.", dll_path.string());
        co_return;
      }

      CORE_LOG_INFO("Successfully built .NET project. Loading assembly from '{}'", dll_path.string());

      auto* env = subsystem<scripting_environment>::get();
      OTHER_ASSERT(env != nullptr, "Scripting environment subsystem is not available.");
      p->set_dotnet_assembly(env->load_dotnet_module(dll_path.string()));
      p->set_state(project::LOADED);
      co_return;
    }

    void process_metadata(const toml::table& table, project::metadata& metadata) {
      toml::node_view md = table.at_path("project.metadata");
      if (md) {
        if (!md.is_array_of_tables()) {
          CORE_LOG_ERROR("Expected 'project.metadata' to be an array of tables.");
          return;
        }
        const auto& metadata_array = md.as_array();
        if (metadata_array->empty()) {
          CORE_LOG_WARN("'project.metadata' array is empty.");
          return;
        }

        for (const auto& item : *metadata_array) {
          if (!item.is_table()) {
            CORE_LOG_ERROR("Expected each item in 'project.metadata' to be a table.");
            continue;
          }
          const auto& metadata_table = item.as_table();
          OTHER_ASSERT(metadata_table != nullptr, "Metadata item is not a table.");

          toml::node_view key = metadata_table->at_path("key");
          toml::node_view value = metadata_table->at_path("value");
          if (!key || !key.is_string()) {
            CORE_LOG_ERROR("Metadata item is missing a valid 'key' field.");
            continue;
          }
          if (!value || !value.is_string()) {
            CORE_LOG_ERROR("Metadata item is missing a valid 'value' field.");
            continue;
          }

          std::string key_str = key.value_or("");
          std::string value_str = value.value_or("");
          if (key_str.empty()) {
            CORE_LOG_ERROR("Metadata item has an empty 'key' field.");
            continue;
          }

          switch (FNV(key_str)) {
            case FNV("name"): metadata.name = value_str; break;
            case FNV("description"): metadata.description = value_str; break;
            case FNV("author"): metadata.author = value_str; break;
            case FNV("version"): metadata.version = value_str; break;
            default:
              CORE_LOG_WARN("Unknown project metadata key '{}'", key_str);
              break;
          }
        }
      }
    }

  }  // namespace detail
}  // namespace other