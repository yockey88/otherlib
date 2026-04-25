/**
 * \file driver/systems/project_system.cpp
 **/
#include "driver/systems/project_system.hpp"

#include <toml++/toml.hpp>

#include "file/file_handle.hpp"
#include "file/filesystem.hpp"

#include "dotnet/dotnet_object.hpp"
#include "script/scripting_environment.hpp"

#include "driver/driver.hpp"

namespace other {

  void project_system::initialize(driver_kernel* kernel) {
    const auto& config = get_driver().configuration();
    if (!config.project_file.has_value()) {
      CORE_LOG_INFO("No project file specified in configuration, skipping project system initialization.");
      return;
    }

    filepath project_file = *config.project_file;
    // command line parser validates existence
    OTHER_ASSERT(std::filesystem::exists(project_file), "Project file '{}' does not exist.", project_file.string());
    OTHER_ASSERT(std::filesystem::is_regular_file(project_file), "Project file '{}' is not a regular file.", project_file.string());
    CORE_LOG_INFO("Loading project from file: '{}'", project_file.string());

    auto* fs = subsystem<file_system>::get();
    OTHER_ASSERT(fs != nullptr, "file_system subsystem is not available.");

    ref<file_handle> handle = fs->register_local_file(project_file.string());
    OTHER_ASSERT(handle != nullptr, "Failed to open project file '{}'", project_file.string());

    std::string file_contents = handle->read_all_as_string();
    process_project_file(file_contents);

    // natural_t size = handle->size();
    // if (size < 0) {
    //   return;
    // }

    // const bool open = handle->open(file_mode::READ);
    // if (!open) {
    //   CORE_LOG_ERROR("Failed to open project file '{}'", project_file.string());
    //   return;
    // }

    // file_buffer.allocate(size);
    // uint64_t bytes_read = handle->read(file_buffer.view_bytes());
    // if (bytes_read != size) {
    //   CORE_LOG_ERROR("Failed to read entire project file '{}'. Expected {} bytes, read {} bytes.", project_file.string(), size, bytes_read);
    //   return;
    // }

    // CORE_LOG_INFO("Successfully loaded project file '{}'", project_file.string());
    // CORE_LOG_DEBUG("Project file size: {} bytes", size);
    // CORE_LOG_DEBUG("Data: {}", file_buffer.dump_buffer());

    // handle->close();
  }

  void project_system::tick(driver_kernel* kernel, double dt) {
  }

  void project_system::shutdown(driver_kernel* kernel) {
  }

  void project_system::process_project_file(const std::string& file_contents) {
    CORE_LOG_DEBUG("Project file contents:\n{}", file_contents);

    try {
      toml::table table = toml::parse(file_contents);
      process_toml(table);
    } catch (const std::exception& e) {
      CORE_LOG_ERROR("Failed to parse project file: {}", e.what());
    } catch (...) {
      CORE_LOG_ERROR("An unknown error occurred while parsing the project file.");
    }
  }

  void project_system::process_toml(const toml::table& table) {
    process_metadata(table);
    {
      std::stringstream ss;
      ss << metadata.name << " v" << metadata.version << " by " << metadata.author;
      if (!metadata.description.empty()) {
        ss << "\n"
           << " - " << metadata.description;
      }
      CORE_LOG_INFO("[PROJECT] {}", ss.str());
    }

    process_scripting_data(table);
  }

  void project_system::process_metadata(const toml::table& metadata_table) {
    toml::node_view md = metadata_table.at_path("project.metadata");
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

  void project_system::process_scripting_data(const toml::table& table) {
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
    scripting_table->for_each([&](const toml::key& key, const toml::node& value) {
      if (key == "cs_project") {
        if (!value.is_string()) {
          CORE_LOG_ERROR("Expected 'cs_project' value to be a string representing the path to the .csproj file.");
        } else {
          CORE_LOG_DEBUG("Attempting to load .NET project: {}", value.as_string()->get());
          load_dotnet_project(value);
        }
      }
    });
  }

  task project_system::load_dotnet_project(const toml::node& node) {
    std::string csproj_path = node.as_string()->get();
    CORE_LOG_INFO("Loading .NET project from '{}'", csproj_path);

    filepath absolute_path = std::filesystem::absolute(filepath(csproj_path));
    if (absolute_path.extension() != ".csproj") {
      CORE_LOG_ERROR("Expected .NET project file to have a .csproj extension, got '{}'", absolute_path.string());
      co_return;
    }

    dotnet_object* builder_obj = nullptr;

    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "scripting_environment subsystem is not available.");

    natural_t builder_id = env->create_object("DotNetProjectBuilder");
    if (builder_id == 0) {
      CORE_LOG_ERROR("Failed to create script object for DotNetProjectBuilder.");
      co_return;
    }

    env->attach_dotnet_object(builder_id, "Other.Scripting.DotNetProjectBuilder");
    script_object* builder_script_obj = env->get_object(builder_id);

    OTHER_ASSERT(builder_script_obj != nullptr, "Failed to retrieve script object for DotNetProjectBuilder with ID {}", builder_id);
    OTHER_ASSERT(builder_script_obj->dotnet_object != nullptr, "DotNetProjectBuilder script object does not have a .NET object attached.");

    CORE_LOG_INFO("Generating .NET project from file '{}'", absolute_path.string());
    builder_script_obj->dotnet_object->set_field("ProjectFilePath", absolute_path.string());

    // builder_obj = env->create_object()
    // if (builder_obj == nullptr) {
    //   CORE_LOG_ERROR("Failed to create instance of 'Other.Scripting.DotNetProjectBuilder'.");
    //   co_return;
    // }

    co_return;
  }

}  // namespace other