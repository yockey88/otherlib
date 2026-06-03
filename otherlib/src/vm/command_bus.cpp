/**
 * \file vm/command_bus.cpp
 **/
#include "vm/command_bus.hpp"

namespace other {

  uint8_t command_bus::register_device(scope<command_device> dev) {
    OTHER_ASSERT(dev != nullptr, "Cannot register null device");

    uint8_t id = dev->device_id();
    OTHER_ASSERT(static_cast<uint16_t>(id) < kMaxDevices, "Device ID out of range");

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
    if (syscall_id >= kMaxDevices) {
      return false;
    }
    if (devices[syscall_id]) {
      devices[syscall_id]->dispatch(static_cast<uint8_t>(syscall_id), device);
      return true;
    }
    return false;
  }

}  // namespace other