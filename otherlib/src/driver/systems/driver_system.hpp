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

  enum driver_system_type : uint32_t {
    EVENT_DRIVER_SYSTEM = 0,
    SCENE_DRIVER_SYSTEM,
    VM_DRIVER_SYSTEM,
    NETWORK_DRIVER_SYSTEM,
    ASSETS_AND_RESOURCES_DRIVER_SYSTEM,
    NUM_BUILTIN_DRIVER_SYSTEMS,

    /*
    /// ???
  debug_overlay  = 0x1000,
  profiler       = 0x1001,
  replay         = 0x1002,
  editor         = 0x1003,
    */

    /// user/custom
    CUSTOM_DRIVER_SYSTEM_ID_START = 0x1000,

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

    virtual void initialize() = 0;
    virtual void tick(float dt) = 0;
    virtual void shutdown() = 0;

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

    void initialize() override;
    void tick(float dt) override;
    void shutdown() override;

    virtual void on_initialize() {}
    virtual void on_tick(float dt) {}
    virtual void on_shutdown() {}

   private:
    bool is_active = false;
  };

  /// CRTP base class for core driver systems
  template <typename D>
  class core_system : public driver_system {
   public:
    core_system(driver* driver_instance, uint32_t id)
        : driver_system(driver_instance, id) {}

    virtual void initialize() override {}
    virtual void tick(float dt) override {}
    virtual void shutdown() override {}

   protected:
    D& self() { return static_cast<D&>(*this); }
    const D& self() const { return static_cast<const D&>(*this); }
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_DRIVER_SYSTEM_HPP