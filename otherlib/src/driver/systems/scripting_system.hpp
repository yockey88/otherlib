/**
 * \file driver/systems/scripting_system.hpp
 **/
#ifndef OTHERLIB_DRIVER_SYSTEMS_SCRIPTING_SYSTEM_HPP
#define OTHERLIB_DRIVER_SYSTEMS_SCRIPTING_SYSTEM_HPP

#include "dotnet/dotnet_assembly.hpp"
#include "lua/lua_host.hpp"

#include "driver/systems/core_system.hpp"

namespace other {

  class lua_script;

  class OTHER_CLASS scripting_system : public core_system<scripting_system> {
   public:
    scripting_system(driver* driver_instance)
        : core_system(driver_instance, static_cast<uint32_t>(driver_system_type::SCRIPTING_DRIVER_SYSTEM)) {}

    std::string name() const override { return "Scripting System"; }

    void initialize(driver_kernel* kernel) override;
    void tick(driver_kernel* kernel, double dt) override;
    void shutdown(driver_kernel* kernel) override;

    ref<assembly> load_dotnet_module(const std::string_view module_path);
    void unload_dotnet_module(ref<assembly> module_id);

    lua_host& get_lua_host();
    lua_script& get_envrc_script();

   private:
    // dotnet_object* dotnet_window_registry = nullptr;
    lua_script* driver_main_lua_script = nullptr;
    lua_script* envrc = nullptr;
    ostd::vector<ref<assembly>> loaded_dotnet_modules;
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_SCRIPTING_SYSTEM_HPP