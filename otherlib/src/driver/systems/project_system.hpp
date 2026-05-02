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

  class project_system : public core_system<project_system> {
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

    void queue_project_load(const filepath& project_file);
    void load_project(driver_kernel* kernel, const filepath& project_file);

    inline project& get_project() {
      OTHER_ASSERT(loaded_project != nullptr, "No project loaded in project system.");
      return *loaded_project;
    }

   private:
    friend class project;

    opt<filepath> next_project_file_to_load;
    opt<filepath> last_loaded_project_file;
    scope<project> loaded_project = nullptr;

    void handle_open_project(driver_kernel* kernel, const value& data);
    void handle_save_project(driver_kernel* kernel, const value& data);
    void handle_new_project(driver_kernel* kernel, const value& data);
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_PROJECT_SYSTEM_HPP