/**
 * \file project/project.cpp
 **/
#include "project/project.hpp"

#include <string_view>

#include <toml++/toml.hpp>

#include "core/defines.hpp"
#include "core/job_system.hpp"
#include "core/weak_ref.hpp"
#include "file/filesystem.hpp"

#include "script/scripting_environment.hpp"

#include "driver/systems/job_driver_system.hpp"
#include "driver/systems/project_system.hpp"

namespace other {
  namespace detail {

    void process_metadata(const toml::table& table, project::metadata& metadata);

  }  // namespace detail

  project::project(project_system* proj_system)
      : system(proj_system) {
    OTHER_ASSERT(system != nullptr, "Project system pointer is null in project constructor.");
  }

  project::~project() {
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

    bool waiting_for_script_load = false;

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

    bool finish_load = false;
    toml::table table;
    try {
      table = toml::parse(file_contents);
      finish_load = true;
    } catch (const std::exception& e) {
      CORE_LOG_ERROR("Failed to parse project file: {}", e.what());
    } catch (...) {
      CORE_LOG_ERROR("An unknown error occurred while parsing the project file.");
    }

    if (!finish_load) {
      CORE_LOG_ERROR("Failed to load project from file '{}'", project_file_handle->virtual_path());
      return;
    }

    detail::process_metadata(table, project_metadata);
    {
      std::stringstream ss;
      ss << project_metadata.name << " v" << project_metadata.version << " by " << project_metadata.author;
      if (!project_metadata.description.empty()) {
        ss << " (" << project_metadata.description << ")";
      }
      CORE_LOG_INFO("[PROJECT] {}", ss.str());
    }

    waiting_for_script_load = process_scripting_sections(table, kernel);
    if (waiting_for_script_load && (project_scripts.csproject_path.empty() || !std::filesystem::exists(project_scripts.csproject_path))) {
      CORE_LOG_ERROR("Project's .NET project file '{}' does not exist. Cannot load project scripts.", project_scripts.csproject_path.string());
      waiting_for_script_load = false;
      project_scripts.csproject_path.clear();
    } else if (waiting_for_script_load) {
      CORE_LOG_INFO("Loading .NET project from '{}'", project_scripts.csproject_path.string());
      system->sibling<asset_system>(*kernel).begin_asset_load(project_scripts.csproject_path);
    }

    process_scene_sections(table);

#else
    static_assert(false, "No project file format defined. define OTHER_PROJECT_FILE_XXX_FORMAT macro.");
#endif

    if (!waiting_for_script_load) {
      set_state(LOADED);
    }
  }

  void project::generate_at(driver_kernel* kernel, const filepath& directory) {
    OTHER_ASSERT(kernel != nullptr, "Driver kernel is null in project generate_at.");
  }

  void project::unload() {
    if (current_state == EMPTY) {
      return;
    }

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

    set_state(EMPTY);
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

  void project::add_built_script(const filepath& script_path) {
    OTHER_ASSERT(std::filesystem::exists(script_path), "Script asset file '{}' does not exist.", script_path.string());
    OTHER_ASSERT(is_loading(), "Project is not in loading state. Cannot add built script.");

    if (script_path.extension() == ".dll") {
      attach_project_dll(script_path);
    } else {
      CORE_LOG_WARN("[PROJECT] Unimplemented script type for built script '{}'.", script_path.string());
    }
  }

  void project::attach_project_cs_file(const filepath& cs_file) {
    OTHER_ASSERT(std::filesystem::exists(cs_file), "C# script file '{}' does not exist.", cs_file.string());
    OTHER_ASSERT(is_loading(), "Project is not in loading state. Cannot attach C# script file.");

    if (cs_file.extension() == ".cs") {
      project_scripts.cs_scripts.push_back(cs_file);
    } else {
      CORE_LOG_WARN("[PROJECT] Unimplemented script type for C# script file '{}'.", cs_file.string());
    }
  }

  void project::add_script_file(const filepath& script_file_path) {
    OTHER_ASSERT(std::filesystem::exists(script_file_path), "Script file '{}' does not exist.", script_file_path.string());
    CORE_LOG_DEBUG("[PROJECT] Adding script file '{}' to project. Script files are not yet fully supported in the project system, so this may not work as expected.", script_file_path.string());

    if (script_file_path.extension() == ".cs") {
      project_scripts.cs_scripts.push_back(script_file_path);
    } else {
      CORE_LOG_WARN("[PROJECT] Unimplemented script type for script file '{}'.", script_file_path.string());
    }
  }

  void project::attach_project_dll(const filepath& dll_path) {
    OTHER_ASSERT(std::filesystem::exists(dll_path), "Project assembly file '{}' does not exist.", dll_path.string());
    OTHER_ASSERT(is_loading(), "Project is not in loading state. Cannot attach project assembly.");

    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "Scripting environment subsystem is not available.");

    ref<assembly> asm_ref = env->get_dotnet_module(dll_path.stem().string());
    OTHER_ASSERT(asm_ref != nullptr, "Failed to load project assembly from '{}'", dll_path.string());

    project_assembly = asm_ref;
    project_scripts.cs_script_source = dll_path;

    set_state(LOADED);
    CORE_LOG_DEBUG("Successfully loaded project assembly from '{}'", dll_path.string());
  }

  bool project::process_scripting_sections(const toml::table& table, driver_kernel* kernel) {
    toml::node_view scripting_node = table.at_path("scripting");
    if (!scripting_node) {
      CORE_LOG_DEBUG("No 'scripting' section found in project file, skipping scripting data processing.");
      return false;
    }
    if (!scripting_node.is_table()) {
      CORE_LOG_ERROR("Expected 'scripting' section to be a table.");
      return false;
    }

    const auto& scripting_table = scripting_node.as_table();
    if (scripting_table == nullptr) {
      CORE_LOG_ERROR("'scripting' section is not a valid table.");
      return false;
    }

    CORE_LOG_DEBUG("Processing project scripting data");

    bool waiting_for_script_load = false;
    scripting_table->for_each([this, &waiting_for_script_load](const toml::key& key, const toml::node& value) {
      std::string k{ key.str() };

      CORE_LOG_DEBUG("Processing scripting entry with key '{}'", k);
      if (k == "cs_project" && value.is_string()) {
        project_scripts.csproject_path = value.as_string()->get();
        waiting_for_script_load = true;
      }
    });

    auto rc_path = scripting_table->get("projectrc-path");
    if (rc_path && rc_path->is_string()) {
      this->rc_path = rc_path->as_string()->get();
      CORE_LOG_DEBUG("Project runtime configuration script path set to '{}'", this->rc_path.string());
    } else {
      CORE_LOG_DEBUG("No 'projectrc-path' specified in project file. Driver environment runtime script will not be loaded.");
    }

    return waiting_for_script_load;
  }

  void project::process_scene_sections(const toml::table& table) {
    PROFILE_SECTION("project::process_scene_sections");

    process_scenes_table(table.at_path("scene-graph.scenes"));
    process_scene_graph(table.at_path("scene-graph.graph"));

    {
      auto starting_scene_node = table.at_path("scene-graph.starting-scene");
      if (starting_scene_node) {
        if (starting_scene_node.is_string()) {
          std::string starting_scene_name = starting_scene_node.as_string()->get();
          auto it = std::ranges::find_if(scenes_in_project, [&starting_scene_name](const scene& s) { return s.name == starting_scene_name; });
          if (it != scenes_in_project.end()) {
            starting_scene_id = it->project_id;
            CORE_LOG_DEBUG("Starting scene set to '{}' with project ID {} based on project file configuration.", it->name, it->project_id);
          } else {
            CORE_LOG_ERROR("Starting scene name '{}' specified in project file does not match any scenes in the project.", starting_scene_name);
          }
        } else if (starting_scene_node.is_number()) {
          starting_scene_id = static_cast<natural_t>(starting_scene_node.as_integer()->get());
          if (std::ranges::none_of(scenes_in_project, [this](const scene& s) { return s.project_id == starting_scene_id; })) {
            natural_t invalid_id = starting_scene_id;
            starting_scene_id = 0;
            CORE_LOG_ERROR("Starting scene ID '{}' specified in project file does not match any scenes in the project.", invalid_id);
          } else {
            CORE_LOG_DEBUG("Starting scene set to project ID {} based on project file configuration.", starting_scene_id);
          }
        } else {
          CORE_LOG_ERROR("Invalid type for 'scene-graph.starting-scene' field. Expected string (scene name) or number (scene ID).");
        }
      }
    }
    CORE_LOG_DEBUG("Finished processing scene sections of project file. Total scenes in project: {}", scenes_in_project.size());
  }

  void project::process_scenes_table(toml::node_view<const toml::node> scenes_node) {
    if (!scenes_node) {
      CORE_LOG_DEBUG("No 'scenes' section found in project file, skipping scene data processing.");
      return;
    }

    if (!scenes_node.is_array_of_tables()) {
      CORE_LOG_ERROR("Expected 'scenes' section to be an array of tables.");
      return;
    }

    const auto* scenes_array = scenes_node.as_array();
    if (scenes_array == nullptr) {
      CORE_LOG_ERROR("'scenes' section is not a valid array of tables.");
      return;
    }

    for (const auto& item : *scenes_array) {
      if (!item.is_table()) {
        CORE_LOG_ERROR("Expected each item in 'scenes' array to be a table.");
        continue;
      }
      const auto* scene_table = item.as_table();
      OTHER_ASSERT(scene_table != nullptr, "Scene item is not a table.");

      project::scene data;

      toml::node_view name_node = scene_table->at_path("name");
      toml::node_view path_node = scene_table->at_path("path");
      toml::node_view id_node = scene_table->at_path("id");

      if (!name_node || !path_node || !id_node) {
        CORE_LOG_ERROR("Scene entry is missing required 'name', 'path', or 'id' field.");
        CORE_LOG_ERROR("!name_node: {}, !path_node: {}, !id_node: {}", !name_node, !path_node, !id_node);
        continue;
      }
      if (!path_node.is_string() || !name_node.is_string() || !id_node.is_integer()) {
        CORE_LOG_ERROR("Scene entry 'name' and 'path' fields must be strings and 'id' field must be an integer.");
        CORE_LOG_ERROR("name_node type: {}, path_node type: {}, id_node type: {}", name_node.type(), path_node.type(), id_node.type());
        continue;
      }

      CORE_LOG_DEBUG(" - Added scene '{}' with path '{}' to project scene list.", data.name, data.path.string());
      scenes_in_project.push_back({
        .name = name_node.as_string()->get(),
        .path = filepath(path_node.as_string()->get()),
        .project_id = static_cast<natural_t>(id_node.as_integer()->get()),
      });
    }

    CORE_LOG_DEBUG("Finished processing 'scenes' section. Total scenes loaded: {}", scenes_in_project.size());
  }

  void project::process_scene_graph(toml::node_view<const toml::node> graph_node) {
    if (!graph_node) {
      CORE_LOG_DEBUG("No 'graph' section found in project file, skipping scene graph processing.");
      return;
    }

    if (!graph_node.is_array_of_tables()) {
      CORE_LOG_ERROR("Expected 'graph' section to be an array of tables.");
      return;
    }

    const auto* graph_array = graph_node.as_array();
    if (graph_array == nullptr) {
      CORE_LOG_ERROR("'graph' section is not a valid array of tables.");
      return;
    }

    for (const auto& item : *graph_array) {
      if (!item.is_table()) {
        CORE_LOG_ERROR("Expected each item in 'graph' array to be a table.");
        continue;
      }
      const auto* graph_table = item.as_table();
      OTHER_ASSERT(graph_table != nullptr, "Graph item is not a table.");

      toml::node_view name_node = graph_table->at_path("id");
      toml::node_view incoming_node = graph_table->at_path("incoming");
      toml::node_view outgoing_node = graph_table->at_path("outgoing");

      if (!name_node || !incoming_node || !outgoing_node) {
        CORE_LOG_ERROR("Graph entry is missing required 'name', 'incoming', or 'outgoing' field.");
        CORE_LOG_ERROR("!name_node: {}, !incoming_node: {}, !outgoing_node: {}", !name_node, !incoming_node, !outgoing_node);
        continue;
      }
      if (!name_node.is_number() || !incoming_node.is_array() || !outgoing_node.is_array()) {
        CORE_LOG_ERROR("Graph entry 'id' must be a number and 'incoming'/'outgoing' must be arrays.");
        CORE_LOG_ERROR("id_node type: {}, incoming_node type: {}, outgoing_node type: {}", name_node.type(), incoming_node.type(), outgoing_node.type());
        continue;
      }

      natural_t scene_id = static_cast<natural_t>(name_node.as_integer()->get());
      auto it = std::ranges::find_if(scenes_in_project, [&scene_id](const project::scene& data) { return data.project_id == scene_id; });
      if (it == scenes_in_project.end()) {
        CORE_LOG_ERROR("Scene with ID '{}' referenced in graph section does not exist in scenes list.", scene_id);
        continue;
      }

      for (const auto& incoming : *incoming_node.as_array()) {
        if (!incoming.is_number()) {
          CORE_LOG_ERROR("Expected 'incoming' array items to be numbers.");
          CORE_LOG_ERROR("incoming item type: {}", incoming.type());
          continue;
        }
        it->incoming.push_back(static_cast<natural_t>(incoming.as_integer()->get()));
      }

      for (const auto& outgoing : *outgoing_node.as_array()) {
        if (!outgoing.is_number()) {
          CORE_LOG_ERROR("Expected 'outgoing' array items to be numbers.");
          CORE_LOG_ERROR("outgoing item type: {}", outgoing.type());
          continue;
        }
        it->outgoing.push_back(static_cast<natural_t>(outgoing.as_integer()->get()));
      }

      CORE_LOG_DEBUG(" - Processed graph entry for scene '{}'. Incoming: {}, Outgoing: {}", scene_id, it->incoming.size(), it->outgoing.size());
    }

    CORE_LOG_DEBUG("Finished processing 'graph' section.");
    CORE_LOG_DEBUG("Scene graph details:");
    for (const auto& scene : scenes_in_project) {
      CORE_LOG_DEBUG(" - Scene '{}': Incoming [{}], Outgoing [{}]", scene.name, scene.incoming, scene.outgoing);
    }
  }

  namespace detail {

    void process_metadata(const toml::table& table, project::metadata& metadata) {
      toml::node_view md = table.at_path("project.metadata");
      if (!md) {
        return;
      }
      if (!md.is_array_of_tables()) {
        CORE_LOG_ERROR("Expected 'project.metadata' to be an array of tables.");
        return;
      }
      const auto* metadata_array = md.as_array();
      if (metadata_array->empty()) {
        CORE_LOG_WARN("'project.metadata' array is empty.");
        return;
      }

      for (const auto& item : *metadata_array) {
        if (!item.is_table()) {
          CORE_LOG_ERROR("Expected each item in 'project.metadata' to be a table.");
          continue;
        }
        const auto* metadata_table = item.as_table();
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

  }  // namespace detail
}  // namespace other