/**
 * \file driver/systems/driver_system.hpp
 **/
#ifndef OTHERLIB_DRIVER_SYSTEMS_DRIVER_SYSTEM_HPP
#define OTHERLIB_DRIVER_SYSTEMS_DRIVER_SYSTEM_HPP

#include <cstdint>

#include "core/defines.hpp"
#include "core/logger.hpp"

namespace other {

  class driver;
  class driver_kernel;

  /**
   * \note important to note that this is also the boot order and update order of the core systems, so they should be ordered with that in mind.
   **/
  enum driver_system_type : uint32_t {
    /// network first because we need to create the io context and register the network system so it is accessible
    NETWORK_DRIVER_SYSTEM = 0,
    /// use IO context from network system to instantiate event system
    EVENT_DRIVER_SYSTEM,
    INPUT_DRIVER_SYSTEM,
    /// VM/asset/scripting/rendering can probably each be launched in any order (or even asynchronously)
    ///    because scene system depends on all of them
    ASSET_DRIVER_SYSTEM,
    SCRIPTING_DRIVER_SYSTEM,
    RENDERING_DRIVER_SYSTEM,
    VM_DRIVER_SYSTEM,
    /// scene system last because it depends on all the others to function properly
    SCENE_DRIVER_SYSTEM,

    /// sentinel
    NUM_BUILTIN_DRIVER_SYSTEMS,

    /// plugin range, these will be launched in increasing ID order (unless a specific dependency is specificied in plugin config)
    CUSTOM_DRIVER_SYSTEM_ID_START = 0x1000,

    // this range reserved for plugin IDs

    CUSTOM_DRIVER_SYSTEM_ID_END = 0xFFFF,
  };
  constexpr static size_t kNumBuiltinDriverSystems = static_cast<size_t>(driver_system_type::NUM_BUILTIN_DRIVER_SYSTEMS);
  constexpr static size_t kNumSystemSlots = kNumBuiltinDriverSystems;

  constexpr static uint32_t kCustomSystemIdStart = static_cast<uint32_t>(driver_system_type::CUSTOM_DRIVER_SYSTEM_ID_START);
  constexpr static uint32_t kCustomSystemIdEnd = static_cast<uint32_t>(driver_system_type::CUSTOM_DRIVER_SYSTEM_ID_END);
  constexpr static uint32_t kNumCustomSystemSlots = kCustomSystemIdEnd - kCustomSystemIdStart + 1;

  struct system_key {
    uint32_t type;
    size_t index = 0;

    constexpr auto operator<=>(const system_key&) const = default;
  };

  class driver_system {
   public:
    driver_system(driver* driver, uint32_t id)
        : driver_instance(driver), system_id(id) {
      OTHER_ASSERT(driver_instance != nullptr, "Driver system must be associated with a driver.");
    }
    virtual ~driver_system() = default;

    virtual bool active() const { return true; }

    uint32_t id() const { return system_id; }
    void force_override_id(uint32_t new_id) { system_id = new_id; }

    virtual std::string name() const = 0;

    virtual void initialize(driver_kernel* kernel) = 0;
    virtual void tick(driver_kernel* kernel, double dt) = 0;
    virtual void shutdown(driver_kernel* kernel) = 0;

   protected:
    driver& get_driver();

   private:
    driver* driver_instance;
    uint32_t system_id;
  };

  class OTHER_CLASS driver_plugin : public driver_system {
   public:
    driver_plugin(driver* driver_instance)
        : driver_system(driver_instance, driver_system_type::CUSTOM_DRIVER_SYSTEM_ID_END) {}
    virtual ~driver_plugin() override = default;

    bool active() const override { return is_active; }
    inline void set_active(bool is_active) { this->is_active = is_active; }

    void initialize(driver_kernel* kernel) override;
    void tick(driver_kernel* kernel, double dt) override;
    void shutdown(driver_kernel* kernel) override;

    virtual void on_initialize() {}
    virtual void on_tick(double dt) {}
    virtual void on_shutdown() {}

   private:
    bool is_active = false;
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_DRIVER_SYSTEM_HPP