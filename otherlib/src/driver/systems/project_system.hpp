/**
 * \file driver/systems/project_system.hpp
 **/
#ifndef OTHERLIB_DRIVER_SYSTEMS_PROJECT_SYSTEM_HPP
#define OTHERLIB_DRIVER_SYSTEMS_PROJECT_SYSTEM_HPP

#include <toml++/toml.hpp>

#include "core/arena_buffer.hpp"
#include "core/coroutine.hpp"

#include "driver/systems/core_system.hpp"

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

   private:
    struct project_metadata {
      std::string name;
      std::string description;
      std::string author;
      std::string version;
    };
    project_metadata metadata;
    arena_buffer file_buffer;

    void process_project_file(const std::string& file_contents);
    void process_toml(const toml::table& table);
    void process_metadata(const toml::table& metadata_table);
    void process_scripting_data(const toml::table& table);

    task load_dotnet_project(const toml::node& node);
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_PROJECT_SYSTEM_HPP