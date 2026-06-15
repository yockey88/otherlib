/**
 * \file vm/devices/asset_device.hpp
 **/
#ifndef OTHERLIB_VM_DEVICES_ASSET_DEVICE_HPP
#define OTHERLIB_VM_DEVICES_ASSET_DEVICE_HPP

#include "vm/builtin_command_devices.hpp"
#include "vm/command_device.hpp"

namespace other {

  class asset_device : public command_device {
   public:
    enum function : uint8_t {
      LOAD_ASSET = 0x00,
      GET_ASSET_STATE = 0x01,
      GET_ASSET_ID = 0x02,
      UNLOAD_ASSET = 0x03,
      RELOAD_ASSET = 0x04,
      NUM_FUNCTIONS,
    };
    ~asset_device() override = default;

    std::string_view device_name() const override { return "asset"; }
    uint8_t device_id() const override { return builtin_command_device_id::OTHER_DEVICE_ASSET; }

    std::span<const function_descriptor> functions() const override;
    void dispatch(uint8_t function_id, other_command_device* device) override;
  };
  static_assert(is_command_device<asset_device>, "asset_device must be a command_device");

}  // namespace other

#endif  // OTHERLIB_VM_DEVICES_ASSET_DEVICE_HPP