/**
 * \file vm/devices/scene_device.hpp
 **/
#ifndef OTHERLIB_VM_DEVICES_SCENE_DEVICE_HPP
#define OTHERLIB_VM_DEVICES_SCENE_DEVICE_HPP

#include "vm/builtin_command_devices.hpp"
#include "vm/command_device.hpp"

namespace other {

  class scene_device : public command_device {
   public:
    enum function : uint8_t {
      LOAD_SCENE = 0x00,
      GET_ACTIVE_SCENE_ID = 0x01,
      FIND_OBJECT = 0x02,
      CREATE_OBJECT = 0x03,
      DESTROY_OBJECT = 0x04,
      SET_OBJECT_VISIBLE = 0x05,
      PLAYBACK_FUNCTION = 0x06,
      GET_SCENE_ID = 0x07,
      NUM_FUNCTIONS,
    };

    std::string_view device_name() const override { return "scene"; }
    uint8_t device_id() const override { return builtin_command_device_id::OTHER_DEVICE_SCENE; };

    std::span<const function_descriptor> functions() const override;
    void dispatch(uint8_t function_id, other_command_device* device) override;
  };
  static_assert(is_command_device<scene_device>, "scene_device must be a command_device");

}  // namespace other

#endif  // OTHERLIB_VM_DEVICES_SCENE_DEVICE_HPP