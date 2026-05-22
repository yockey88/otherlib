/**
 * \file driver/driver_kernel.hpp
 **/
#ifndef OTHERLIB_DRIVER_DRIVER_KERNEL_HPP
#define OTHERLIB_DRIVER_DRIVER_KERNEL_HPP

#include "core/arena_allocator.hpp"
#include "core/logger.hpp"

#include "driver/driver_system.hpp"
#include "driver/environment_registry.hpp"
#include "driver/systems/driver_plugin.hpp"
#include "plugin/plugin_manifest.hpp"

namespace other {

  class driver;
  class library_handle;

  class OTHER_CLASS driver_kernel {
   public:
    driver_kernel(driver* driver_instance)
        : driver_instance(driver_instance) {}
    virtual ~driver_kernel() = default;

    void load_profile(const std::string_view profile_name);
    void load_driver_plugins_from_config(driver* driver_instance);
    void initialize();
    void tick(double dt);
    void unload_project_plugins();
    void unload_driver_plugins();
    void unload_plugins();
    void shutdown();

    void update_order();

    std::string list_systems() const;

    void register_project_plugin(const filepath& plugin_name, library_handle* plugin_library);

    template <typename T, typename... Args>
      requires std::derived_from<T, driver_system>
    [[maybe_unused]] T& add_system(driver_system_type type, Args&&... args) {
      static_assert(std::derived_from<T, driver_system>, "Added system must derive from driver_system");
      OTHER_ASSERT(driver_instance != nullptr, "Driver kernel is not associated with a driver.");

      size_t type_id = typeid(T).hash_code();
      OTHER_ASSERT(registered_core_systems.find(type_id) == registered_core_systems.end(), "Core system of type {} is already registered.", typeid(T).name());
      registered_core_systems[type_id] = type;
      CORE_LOG_DEBUG("Core system registered: {} with type id {}", typeid(T).name(), type_id);

      T* system = arena_allocator<T>{}.allocate(driver_instance, std::forward<Args>(args)...);
      OTHER_ASSERT(system != nullptr, "Failed to allocate system of type {}", typeid(T).name());
      builtin_systems[static_cast<size_t>(type)] = system;

      return *system;
    }

    template <typename T>
    T& get_core_system() {
      auto id = typeid(T).hash_code();
      auto it = registered_core_systems.find(id);
      OTHER_ASSERT(it != registered_core_systems.end(), "Core system of type {} is not registered in kernel.", typeid(T).name());

      driver_system_type type = it->second;
      driver_system* system = builtin_systems[static_cast<size_t>(type)];
      OTHER_ASSERT(system != nullptr, "Core system of type {} is not initialized in kernel.", typeid(T).name());
      T* casted_system = dynamic_cast<T*>(system);
      OTHER_ASSERT(casted_system != nullptr, "Failed to cast core system of type {} to type {}", typeid(T).name(), typeid(T).name());
      return *casted_system;
    }

    template <typename T>
    const T& get_core_system() const {
      return get_core_system<T>();
    }

    template <typename T>
    bool has_core_system() const {
      auto id = typeid(T).hash_code();
      return registered_core_systems.find(id) != registered_core_systems.end();
    }

    void remove_system(driver_system_type type);

    void shutdown_plugin(driver_system* plugin);
    void remove_plugin(uint32_t id, size_t index);

    driver_plugin* get_plugin(uint32_t id, size_t index = 0);

    template <typename T>
    T* get_plugin(uint32_t id, size_t index = 0) {
      driver_system* plugin = get_plugin(id, index);
      if (plugin == nullptr) {
        return nullptr;
      }
      T* casted_plugin = dynamic_cast<T*>(plugin);
      if (casted_plugin == nullptr) {
        CORE_LOG_ERROR("Failed to cast plugin with id {} and index {} to type {}", id, index, typeid(T).name());
        return nullptr;
      }
      return casted_plugin;
    }

    template <typename T>
    T* get_builtin_system(driver_system_type type) {
      driver_system* system = builtin_systems[static_cast<size_t>(type)];
      if (system == nullptr) {
        CORE_LOG_ERROR("Builtin system of type {} is not initialized.", static_cast<uint32_t>(type));
        return nullptr;
      }
      T* casted_system = dynamic_cast<T*>(system);
      if (casted_system == nullptr) {
        CORE_LOG_ERROR("Failed to cast builtin system of type {} to type {}", static_cast<uint32_t>(type), typeid(T).name());
        return nullptr;
      }
      return casted_system;
    }

    inline environment_registry& get_environment_registry(interface_scope scope) {
      return environment_registries[static_cast<size_t>(scope)].registry;
    }
    inline environment_registry& global_registry() { return get_environment_registry(interface_scope::GLOBAL); }
    inline environment_registry& driver_registry() { return get_environment_registry(interface_scope::DRIVER); }
    inline environment_registry& project_registry() { return get_environment_registry(interface_scope::PROJECT); }

   private:
    driver* driver_instance;

    std::map<size_t, driver_system_type> registered_core_systems;
    std::vector<driver_system_type> system_order;

    std::map<system_key, std::string> plugin_name;
    std::map<system_key, driver_system*> plugin_systems;
    std::array<driver_system*, kNumBuiltinDriverSystems> builtin_systems{};

    struct plugin_library_info {
      std::string library_name;
      natural_t plugin_id;
    };

    struct plugin_registry {
      environment_registry registry;
      std::vector<plugin_library_info> provided_plugins;

      plugin_registry(interface_scope scope)
          : registry(scope) {}
    };

    std::array<plugin_registry, kNumInterfaceScopes> environment_registries{
      plugin_registry(interface_scope::GLOBAL),
      plugin_registry(interface_scope::DRIVER),
      plugin_registry(interface_scope::PROJECT),
    };

    template <typename T>
      requires std::derived_from<T, driver_system>
    T* builtin_system_as(driver_system_type type) {
      OTHER_ASSERT(type < kNumBuiltinDriverSystems, "Invalid builtin system type: {}", static_cast<uint32_t>(type));
      driver_system* system = builtin_systems[static_cast<size_t>(type)];
      OTHER_ASSERT(system != nullptr, "Builtin system of type {} is not initialized.", static_cast<uint32_t>(type));
      T* casted_system = dynamic_cast<T*>(system);
      OTHER_ASSERT(casted_system != nullptr, "Failed to cast builtin system of type {} to type {}", static_cast<uint32_t>(type), typeid(T).name());
      return casted_system;
    }

    void register_plugin(plugin_registry& registry, const filepath& path, library_handle* lib);
    void register_driver_plugin(const filepath& path, library_handle* lib);
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_DRIVER_KERNEL_HPP