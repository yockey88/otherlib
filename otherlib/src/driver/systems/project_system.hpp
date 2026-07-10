/**
 * \file driver/systems/project_system.hpp
 **/
#ifndef OTHERLIB_DRIVER_SYSTEMS_PROJECT_SYSTEM_HPP
#define OTHERLIB_DRIVER_SYSTEMS_PROJECT_SYSTEM_HPP

#include <toml++/toml.hpp>

#include "core/defines.hpp"

#include "driver/systems/core_system.hpp"
#include "project/project.hpp"

namespace other {

  class OTHER_CLASS project_system : public core_system<project_system> {
   public:
    project_system(driver* driver_instance)
        : core_system(driver_instance, driver_system_type::PROJECT_DRIVER_SYSTEM) {}
    virtual ~project_system() = default;

    std::string name() const override { return "Project System"; }

    void initialize(driver_kernel* kernel) override;
    void tick(driver_kernel* kernel, double dt) override;
    void shutdown(driver_kernel* kernel) override;

    bool project_empty() const;
    bool project_loading() const;
    bool project_loaded() const;
    bool project_unloading() const;

    void generate_project_at(driver_kernel* kernel, const filepath& directory);
    void load_project(driver_kernel* kernel, const filepath& project_file);
    void unload_project(driver_kernel* kernel);
    bool is_project_empty() const;
    bool is_project_loading() const;
    bool is_project_unloading() const;
    bool is_project_loaded() const;

    void handle_project_event(driver_kernel* kernel, const project_event_data& data);

    inline project& get_project() {
      OTHER_ASSERT(loaded_project != nullptr, "No project loaded in project system.");
      return *loaded_project;
    }

   private:
    friend class project;

    opt<filepath> next_project_file_to_load;
    opt<filepath> last_loaded_project_file;
    scope<project> loaded_project = nullptr;

    void handle_new_project(driver_kernel* kernel, const value& data);
    void handle_open_project(driver_kernel* kernel, const value& data);
    void handle_save_project(driver_kernel* kernel, const value& data);

    void handle_script_project_loaded(driver_kernel* kernel, const value& data);
    void handle_script_project_load_failed(driver_kernel* kernel, const value& data);
    void handle_script_source_loaded(driver_kernel* kernel, const value& data);
    void handle_script_source_load_failed(driver_kernel* kernel, const value& data);
    void handle_script_source_unloaded(driver_kernel* kernel, const value& data);
    void handle_script_file_loaded(driver_kernel* kernel, const value& data);
    void handle_script_file_unloaded(driver_kernel* kernel, const value& data);

    void load_plugin(const std::string& plugin_name, const filepath& plugin_path);
    void unload_plugins();
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_PROJECT_SYSTEM_HPP