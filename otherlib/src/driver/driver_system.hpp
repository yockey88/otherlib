/**
 * \file driver/systems/driver_system.hpp
 **/
#ifndef OTHERLIB_DRIVER_SYSTEMS_DRIVER_SYSTEM_HPP
#define OTHERLIB_DRIVER_SYSTEMS_DRIVER_SYSTEM_HPP

#include <cstdint>

#include "core/logger.hpp"

namespace other {

  class driver;
  class driver_kernel;

  /** \note also defines core-system boot/update order — keep consistent
   **/
  enum driver_system_type : uint32_t {
    /// group 0
    NETWORK_DRIVER_SYSTEM = 0,
    JOB_DRIVER_SYSTEM,
    /// group 1
    EVENT_DRIVER_SYSTEM,
    /// group 2 - might need to use events
    INPUT_DRIVER_SYSTEM,
    ASSET_DRIVER_SYSTEM,
    /// group3 - needs assets, and depends on inputs for certain things
    SCRIPTING_DRIVER_SYSTEM,
    RENDERING_DRIVER_SYSTEM,
    /// group4 - primary systems managing core objects
    VM_DRIVER_SYSTEM,
    SCENE_DRIVER_SYSTEM,
    /// after SCENE so voices read the frame's final world transforms
    AUDIO_DRIVER_SYSTEM,
    PROJECT_DRIVER_SYSTEM,

    /// sentinel
    NUM_BUILTIN_DRIVER_SYSTEMS,

    /// plugin range [0x1000, 0xFFFF]
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

  class OTHER_CLASS driver_system {
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
    virtual void late_initialize(driver_kernel* kernel) {}
    virtual void on_driver_ready(driver_kernel* kernel) {}
    virtual void tick(driver_kernel* kernel, double dt) = 0;
    virtual void shutdown(driver_kernel* kernel) = 0;

    driver& get_driver();

   private:
    driver* driver_instance;
    uint32_t system_id;
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_DRIVER_SYSTEM_HPP