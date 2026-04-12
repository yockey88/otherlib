/**
 * \file driver/driver_kernel.hpp
 **/
#ifndef OTHERLIB_DRIVER_DRIVER_KERNEL_HPP
#define OTHERLIB_DRIVER_DRIVER_KERNEL_HPP

#include <map>

#include "core/arena_allocator.hpp"
#include "core/logger.hpp"

#include "driver/systems/driver_system.hpp"

namespace other {

  class driver_kernel;

  /// CRTP base class for core driver systems
  template <typename D>
  class core_system : public driver_system {
   public:
    core_system(driver* driver_instance, uint32_t id)
        : driver_system(driver_instance, id) {}

   protected:
    /// convenience: access a sibling system through the kernel implemented after driver_kernel is defined
    template <typename T>
    T& sibling(driver_kernel& kernel);

    template <typename T>
    const T& sibling(const driver_kernel& kernel) const;

    template <typename T>
    bool has_sibling(const driver_kernel& kernel) const;
  };

  class driver_kernel {
   public:
    driver_kernel(driver* driver_instance)
        : driver_instance(driver_instance) {}
    virtual ~driver_kernel() = default;

    void initialize();
    void tick(float dt);
    void shutdown();

    void update_order();

    template <typename T, typename... Args>
    T& add_system(driver_system_type type, Args&&... args) {
      static_assert(std::derived_from<T, driver_system>, "Added system must derive from driver_system");
      OTHER_ASSERT(driver_instance != nullptr, "Driver kernel is not associated with a driver.");

      size_t type_id = typeid(T).hash_code();
      OTHER_ASSERT(registered_core_systems.find(type_id) == registered_core_systems.end(), "Core system of type {} is already registered.", typeid(T).name());
      registered_core_systems[type_id] = type;

      T* system = arena_allocator<T>{}.allocate(driver_instance, std::forward<Args>(args)...);
      OTHER_ASSERT(system != nullptr, "Failed to allocate system of type {}", typeid(T).name());

      builtin_systems[static_cast<size_t>(type)] = system;
      system->initialize(this);
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

    template <typename T, typename... Args>
    T* install_plugin(Args&&... args) {
      static_assert(std::derived_from<T, driver_plugin>, "Installed addon must derive from driver_plugin");
      OTHER_ASSERT(driver_instance != nullptr, "Driver kernel is not associated with a driver.");
      T* addon = arena_allocator<T>{}.allocate(driver_instance, std::forward<Args>(args)...);
      OTHER_ASSERT(addon != nullptr, "Failed to allocate addon of type {}", typeid(T).name());

      // OTHER_AS

      return static_cast<T*>(install_plugin(addon));
    }
    driver_system* install_plugin(driver_system* plugin);

    void remove_plugin(uint32_t id, size_t index);
    void remove_plugin(const std::string_view name);
    void remove_plugin(driver_system* plugin);

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

   private:
    driver* driver_instance;

    std::map<size_t, driver_system_type> registered_core_systems;
    std::vector<driver_system_type> system_order;

    std::map<system_key, driver_system*> plugin_systems;
    std::array<driver_system*, kNumBuiltinDriverSystems> builtin_systems{};

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

    void tick_plugins(float dt);
    void shutdown_plugins();
  };

  template <typename D>
  template <typename T>
  T& core_system<D>::sibling(driver_kernel& kernel) {
    return kernel.get_core_system<T>();
  }

  template <typename D>
  template <typename T>
  const T& core_system<D>::sibling(const driver_kernel& kernel) const {
    return kernel.get_core_system<T>();
  }

  template <typename D>
  template <typename T>
  bool core_system<D>::has_sibling(const driver_kernel& kernel) const {
    return kernel.has_core_system<T>();
  }

}  // namespace other

#endif  // OTHERLIB_DRIVER_DRIVER_KERNEL_HPP