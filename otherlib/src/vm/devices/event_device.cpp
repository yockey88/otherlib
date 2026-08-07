/**
 * \file vm/devices/event_device.cpp
 **/
#include "vm/devices/event_device.hpp"

#include "core/profiler.hpp"
#include "event/event_system.hpp"

#include "driver/driver.hpp"
#include "vm/device_utils.hpp"
#include "vm/vm.hpp"
#include "vm/vm_error.hpp"

namespace other {
  namespace {

    constexpr type_descriptor kTriggerEventParams[] = {
      { VM_TYPE_PTR, "event-name-ptr" },
      { VM_TYPE_U64, "event-name-len" },
    };
    constexpr type_descriptor kVoidRet = { VM_TYPE_VOID, "void" };

    constexpr function_descriptor kFunctions[] = {
      { event_device::TRIGGER_EVENT, "trigger_event", kTriggerEventParams, kVoidRet, VM_SE_ANYTHING_POSSIBLE },
    };

    constexpr logger::level to_log_level(uint64_t level) {
      constexpr uint64_t kMaxLevel = static_cast<uint64_t>(logger::level::CRITICAL);
      return static_cast<logger::level>(std::min(level, kMaxLevel));
    }

  }  // namespace

  std::span<const function_descriptor> event_device::functions() const {
    return kFunctions;
  }

  void event_device::dispatch(uint8_t function_id, other_command_device* device) {
    PROFILE_SECTION("event_device::dispatch");
    switch (function_id) {
      case TRIGGER_EVENT: {
        std::string event_name = detail::read_device_string(device, vm_register_idx::VM_R0, vm_register_idx::VM_R1);
        if (device->read_flag_register() != vm_error::VM_OK) {
          break;
        }
        device->host_driver->get_event_system()->trigger_event(event_name);
      } break;
      default:
        // Handle unknown function_id
        break;
    }
  }

}  // namespace other