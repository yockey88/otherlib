/**
 * \file vm/devices/core_command_device.hpp
 **/
#ifndef OTHERLIB_VM_DEVICES_CORE_COMMAND_DEVICE_HPP
#define OTHERLIB_VM_DEVICES_CORE_COMMAND_DEVICE_HPP

#include "vm/builtin_command_devices.hpp"
#include "vm/command_device.hpp"

namespace other {

  class core_command_device final : public command_device {
   public:
    enum function : uint8_t {
      LOG = 0x00,          // r0 = msg ptr, r1 = msg len, r2 = level
      LOG_U64 = 0x01,      // r0 = value, r1 = level
      TIME_MICROS = 0x02,  //                          -> rF = us since vm init
      RANDOM = 0x03,       //                          -> rF = uniform u64
      ERROR_NAME = 0x04,   // r0 = code, r1 = dst ptr, r2 = cap -> rF = len
      NUM_FUNCTIONS,
    };
    ~core_command_device() override = default;

    std::string_view device_name() const override { return "CORE"; }
    uint8_t device_id() const override { return builtin_command_device_id::OTHER_DEVICE_CORE; }

    std::span<const function_descriptor> functions() const override;
    void dispatch(uint8_t function_id, other_command_device* device) override;
  };
  static_assert(is_command_device<core_command_device>, "core_command_device must satisfy is_command_device concept");

}  // namespace other

#endif  // OTHERLIB_VM_DEVICES_CORE_COMMAND_DEVICE_HPP