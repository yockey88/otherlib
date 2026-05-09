/**
 * \file project/project.hpp
 **/
#ifndef OTHERLIB_PROJECT_PROJECT_HPP
#define OTHERLIB_PROJECT_PROJECT_HPP

#include <toml++/toml.hpp>

#include "core/arena_buffer.hpp"
#include "core/defines.hpp"
#include "file/file_handle.hpp"

#include "dotnet/dotnet_assembly.hpp"

#include "tools/project_tool.hpp"

namespace other {

  class driver_kernel;
  class project_system;

  struct project_event_data {
    std::string type;
    std::string project_name;
    std::string project_path;
  };
  class project {
   public:
    enum state {
      EMPTY = 0,
      LOADING,
      LOADED,
      UNLOADING,

      NUM_STATES,
      INVALID_STATE = NUM_STATES,
    };
    struct metadata {
      std::string name;
      std::string description;
      std::string author;
      std::string version;
    };
    struct scene {
      std::string name;
      filepath path;
      natural_t project_id;
      natural_t scene_id;

      std::vector<natural_t> incoming;
      std::vector<natural_t> outgoing;
    };
    struct script_data {
      filepath csproject_path;
      filepath cs_script_source;
      std::vector<filepath> cs_scripts;
    };

    project(project_system* proj_system);
    ~project();

    void load_from_file(driver_kernel* kernel, const filepath& path);
    void generate_at(driver_kernel* kernel, const filepath& directory);

    void unload();
    void set_state(state new_state);

    void add_built_script(const filepath& script_asset_path);
    void add_script_file(const filepath& script_file_path);

    inline const filepath& get_project_rc_path() const { return rc_path; }

    inline void set_dotnet_assembly(ref<assembly> a) { project_assembly = a; }
    inline state get_state() const { return current_state; }
    inline bool is_empty() const { return current_state == EMPTY; }
    inline bool is_loaded() const { return current_state == LOADED; }
    inline bool is_loading() const { return current_state == LOADING; }
    inline bool is_unloading() const { return current_state == UNLOADING; }

    inline natural_t get_starting_scene_id() const { return starting_scene_id; }
    inline std::vector<scene>& get_scenes() { return scenes_in_project; }
    inline const std::vector<scene>& get_scenes() const { return scenes_in_project; }

   private:
    struct project_args {
      std::string name;
      std::string working_directory;
    };

    state current_state = EMPTY;

    project_system* system = nullptr;
    metadata project_metadata;
    arena_buffer file_buffer;

    filepath rc_path;
    ref<file_handle> project_file_handle;
    ref<assembly> project_assembly;

    natural_t starting_scene_id = 0;
    std::vector<scene> scenes_in_project;
    script_data project_scripts;

    void attach_project_dll(const filepath& dll_path);
    void attach_project_cs_file(const filepath& cs_file);

    bool process_scripting_sections(const toml::table& table, driver_kernel* kernel);
    void process_scene_sections(const toml::table& table);
    void process_scenes_table(toml::node_view<const toml::node> scenes_node);
    void process_scene_graph(toml::node_view<const toml::node> graph_node);
  };

  // struct project_description {
  //   enum type : uint16_t {
  //     APPLICATION = 0,
  //     MODULE,

  //     NUM_PROJECT_TYPES,
  //     INVALID_PROJECT_TYPE = NUM_PROJECT_TYPES,
  //   };
  //   type project_type = APPLICATION;
  //   std::string project_name = "NewProject";

  //   filepath environment_config = "${project-directory}/${project-name}.toml";
  //   filepath working_directory = "${project-directory}";
  //   filepath output_directory = "${project-directory}/build";
  //   filepath exe_name = "${project-directory}/${project-name}.exe";

  //   std::vector<std::string> configurations = { "Debug", "Release" };

  //   size_t active_configuration = 0;
  //   std::vector<std::string> cmd_args = {};

  //   std::string version = "0.1.0";
  //   std::string description = "An Other project.";
  //   std::string author = "Author Name";

  //   std::string license = "MIT";
  // };

}  // namespace other

#endif  // OTHERLIB_PROJECT_PROJECT_HPP