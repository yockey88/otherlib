/**
 * \file vm/device_utils.hpp
 **/
#ifndef OTHERLIB_VM_DEVICE_UTILS_HPP
#define OTHERLIB_VM_DEVICE_UTILS_HPP

namespace other {

  struct other_command_device;

  namespace detail {

    std::string read_device_string(other_command_device* device, uint8_t ptr_reg, uint8_t len_reg);
    uint64_t write_device_bytes(other_command_device* device, uint64_t offset, std::span<const uint8_t> bytes, uint64_t capacity);

  }  // namespace detail
}  // namespace other

#endif  // OTHERLIB_VM_DEVICE_UTILS_HPP