/**
 * \file vm/command_bus.hpp
 **/
#ifndef OTHERLIB_VM_COMMAND_BUS_HPP
#define OTHERLIB_VM_COMMAND_BUS_HPP

#include "core/scope.hpp"

#include "vm/command_device.hpp"
#include "vm/command_files/symbol_resolver.hpp"

namespace other {

  struct other_command_bus;

  class command_bus {
   public:
    uint8_t register_device(scope<command_device> dev);
    command_device* find_device(uint8_t id) const;
    command_device* find_device_by_name(std::string_view) const;

    bool dispatch(uint16_t syscall_id, other_command_device* device);

    scope<symbol_resolver> create_default_symbol_resolver() const;

    constexpr static uint16_t kMaxDevices = 0xFF + 1;

    static uint16_t create_syscall_id(uint8_t device_id, uint8_t function_id) {
      return (static_cast<uint16_t>(device_id) << 8) | function_id;
    }

   private:
    command_device* devices[kMaxDevices] = {};  // indexed by device id
    scope<command_device> owned[kMaxDevices];
  };

}  // namespace other

#endif  // OTHERLIB_VM_COMMAND_BUS_HPP