/**
 * \file vm/builtin_command_devices.hpp
 **/
#ifndef OTHERLIB_VM_BUILTIN_COMMAND_DEVICES_HPP
#define OTHERLIB_VM_BUILTIN_COMMAND_DEVICES_HPP

namespace other {

  enum builtin_command_device_id : uint8_t {
    OTHER_DEVICE_CORE = 0x00,
    OTHER_DEVICE_EVENT = 0x01,
    OTHER_DEVICE_ASSET = 0x02,
    OTHER_DEVICE_SCENE = 0x03,
    OTHER_DEVICE_PROJECT = 0x04,

    /// constant to mark beginning of user devices
    OTHER_DEVICE_USER_RANGE_START = 0x80,
  };

}  // namespace other

#endif  // OTHERLIB_VM_BUILTIN_COMMAND_DEVICES_HPP