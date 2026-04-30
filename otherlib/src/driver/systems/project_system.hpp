/**
 * \file driver/systems/project_system.hpp
 **/
#ifndef OTHERLIB_DRIVER_SYSTEMS_PROJECT_SYSTEM_HPP
#define OTHERLIB_DRIVER_SYSTEMS_PROJECT_SYSTEM_HPP

#include <toml++/toml.hpp>

#include "core/arena_buffer.hpp"
#include "core/coroutine.hpp"
#include "core/defines.hpp"
#include "file/file_handle.hpp"

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

    inline project& get_project() {
      OTHER_ASSERT(loaded_project != nullptr, "No project loaded in project system.");
      return *loaded_project;
    }

   private:
    friend class project;

    scope<project> loaded_project = nullptr;
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_PROJECT_SYSTEM_HPP