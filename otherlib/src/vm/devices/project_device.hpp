/**
 * \file vm/devices/project_device.hpp
 **/
#ifndef OTHERLIB_VM_DEVICES_PROJECT_DEVICE_HPP
#define OTHERLIB_VM_DEVICES_PROJECT_DEVICE_HPP

#include "vm/builtin_command_devices.hpp"
#include "vm/command_device.hpp"


namespace other {

  class project_device : public command_device {
   public:
    enum function : uint8_t {
      NUM_FUNCTIONS
    };
    ~project_device() override = default;

    std::string_view device_name() const override { return "project"; }
    uint8_t device_id() const override { return builtin_command_device_id::OTHER_DEVICE_PROJECT; }

    std::span<const function_descriptor> functions() const override;
    void dispatch(uint8_t function_id, other_command_device* device) override;
  };
  static_assert(is_command_device<project_device>, "project_device must be a command_device");

}  // namespace other

#endif  // OTHERLIB_VM_DEVICES_PROJECT_DEVICE_HPP