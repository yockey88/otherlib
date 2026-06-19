/**
 * \file vm/devices/event_device.hpp
 **/
#ifndef OTHERLIB_VM_DEVICES_EVENT_DEVICE_HPP
#define OTHERLIB_VM_DEVICES_EVENT_DEVICE_HPP

#include "vm/builtin_command_devices.hpp"
#include "vm/command_device.hpp"

namespace other {

  class event_device : public command_device {
   public:
    enum function : uint8_t {
      TRIGGER_EVENT = 0x00,
      NUM_FUNCTIONS,
    };
    ~event_device() override = default;

    std::string_view device_name() const override { return "event"; }
    uint8_t device_id() const override { return builtin_command_device_id::OTHER_DEVICE_EVENT; }

    std::span<const function_descriptor> functions() const override;
    void dispatch(uint8_t function_id, other_command_device* device) override;
  };
  static_assert(is_command_device<event_device>, "event_device must satisfy is_command_device concept");

}  // namespace other

#endif  // OTHERLIB_VM_DEVICES_EVENT_DEVICE_HPP