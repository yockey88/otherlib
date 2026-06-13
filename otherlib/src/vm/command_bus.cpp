/**
 * \file vm/command_bus.cpp
 **/
#include "vm/command_bus.hpp"

#include "vm/default_symbol_resolver.hpp"
#include "vm/opcode.hpp"
#include "vm/other_device.hpp"

namespace other {

  uint8_t command_bus::register_device(scope<command_device> dev) {
    OTHER_ASSERT(dev != nullptr, "Cannot register null device");

    uint8_t id = dev->device_id();
    OTHER_ASSERT(static_cast<uint16_t>(id) < kMaxDevices, "Device ID out of range");
    OTHER_ASSERT(devices[id] == nullptr, "Device id {:#04x} already registered ('{}')",
                 id, devices[id] != nullptr ? devices[id]->device_name() : "");

    devices[id] = dev.get();
    owned[id] = std::move(dev);

    return id;
  }

  command_device* command_bus::find_device(uint8_t id) const {
    if (static_cast<uint16_t>(id) >= kMaxDevices) {
      return nullptr;
    }
    return devices[id];
  }

  command_device* command_bus::find_device_by_name(std::string_view name) const {
    for (uint16_t i = 0; i < kMaxDevices; ++i) {
      if (devices[i] && devices[i]->device_name() == name) {
        return devices[i];
      }
    }
    return nullptr;
  }

  bool command_bus::dispatch(uint16_t syscall_id, other_command_device* device) {
    OTHER_ASSERT(device != nullptr, "Device cannot be null for dispatch");
    if (syscall_id >= kMaxDevices) {
      return false;
    }

    CORE_LOG_DEBUG("[VM] Attempting to execute syscall with ID {:#06x} at PC {:#06x}", syscall_id, device->pc);
    const syscall sc{ syscall_id };
    command_device* target_device = find_device(sc.device);
    if (target_device == nullptr) {
      CORE_LOG_ERROR("[VM] No device found for syscall ID {:#06x}", sc.device);
      return false;
    }

    CORE_LOG_DEBUG("[VM] - executing syscall ID {:#06x}", syscall_id);
    CORE_LOG_DEBUG("[VM] - device '{}' (ID {:#04x})", target_device->device_name(), sc.device);
    target_device->dispatch(sc.function, device);
    return true;
  }

  scope<symbol_resolver> command_bus::create_default_symbol_resolver() const {
    auto res = make_scope<default_symbol_resolver>();

    for (uint16_t i = 0; i < kMaxDevices; ++i) {
      if (devices[i]) {
        std::string device_name{ devices[i]->device_name() };
        auto funcs = devices[i]->functions();
        for (const auto& func : funcs) {
          std::string symbol = std::format("{}.{}", device_name, func.name);
          CORE_LOG_DEBUG("[COMMAND BUS] Registering symbol '{}'", symbol);
          res->register_symbol(symbol);
          res->attach_code_label(symbol, create_syscall_id(i, func.id));
        }
      }
    }

    return res;
  }

}  // namespace other