/**
 * \file driver/subsystem_registry.hpp
 **/
#ifndef OTHERLIB_DRIVER_SUBSYSTEM_REGISTRY_HPP
#define OTHERLIB_DRIVER_SUBSYSTEM_REGISTRY_HPP

#include <string>
#include <unordered_map>
#include <vector>

#include "core/config_table.hpp"
#include "core/subsystem.hpp"

namespace other {

  class subsystem_registry {
   public:
    void register_subsystem(const subsystem_definition& def);
    void initialize_profile(const std::string_view profile, const config_table* config);
    void resolve_dependency_list_and_do_initialization(std::span<const std::string_view> requested_systems, const config_table* config);
    void shutdown_all(bool skip_logger);

    static bool profile_includes_scripting(const std::string_view profile_name);
    static bool profile_includes_physics(const std::string_view profile_name);
    static bool profile_includes_rendering(const std::string_view profile_name);
    static bool profile_includes_audio(const std::string_view profile_name);
    static bool profile_includes_vm(const std::string_view profile_name);
    static bool profile_includes_scene(const std::string_view profile_name);

    inline const std::string_view get_current_profile() const { return current_profile; }
    const std::vector<natural_t>& get_initialization_order() const { return initialization_order; }
    const std::unordered_map<natural_t, subsystem_definition>& get_registry() const { return registry; }

   private:
    std::string_view current_profile = "";
    std::vector<natural_t> initialization_order;
    std::unordered_map<natural_t, subsystem_definition> registry;

    void activate_necessary_subsystems_for_profile(const std::string_view profile, const config_table* config);
    std::vector<natural_t> resolve_dependencies(std::span<const std::string_view> requested_systems, const subsystem_definition& def);
  };

  subsystem_registry register_all_subsystems();
  std::string get_subsystem_profile(const config_table* config);
  std::span<const std::string_view> get_required_subsystems_for_profile(const std::string_view profile_name);

}  // namespace other

#endif  // OTHERLIB_DRIVER_SUBSYSTEM_REGISTRY_HPP