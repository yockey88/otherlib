/**
 * \file vm/command_device.hpp
 **/
#ifndef OTHERLIB_VM_COMMAND_DEVICE_HPP
#define OTHERLIB_VM_COMMAND_DEVICE_HPP

#include <concepts>

#include "vm/vm_function.hpp"

namespace other {

  struct other_command_device;

  class command_device {
   public:
    virtual ~command_device() = default;

    virtual std::string_view device_name() const = 0;
    virtual uint8_t device_id() const = 0;

    virtual std::span<const function_descriptor> functions() const = 0;
    virtual void dispatch(uint8_t function_id, other_command_device* device) = 0;

    virtual bool ready() const { return true; }
  };

  template <typename T>
  concept is_command_device =
    std::derived_from<T, command_device> &&
    requires(T t) {
      { T::function::NUM_FUNCTIONS } -> std::convertible_to<uint8_t>;
    };

}  // namespace other

#endif  // OTHERLIB_VM_COMMAND_DEVICE_HPP