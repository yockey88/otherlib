/**
 * \file vm/device_utils.cpp
 **/
#include "vm/device_utils.hpp"

#include "core/arena.hpp"
#include "core/logger.hpp"

#include "vm/other_device.hpp"
#include "vm/vm.hpp"
#include "vm/vm_error.hpp"

namespace other {
  namespace detail {

    std::string read_device_string(other_command_device* device, uint8_t ptr_reg, uint8_t len_reg) {
      OTHER_ASSERT(device != nullptr, "Device cannot be null for reading string");
      OTHER_ASSERT(device->memory != nullptr, "Device memory cannot be null for reading string");

      uint64_t msg_ptr = device->read_register_as_u64(ptr_reg);
      uint64_t msg_len_ptr = device->read_register_as_u64(len_reg);
      CORE_LOG_DEBUG("[VM] Reading string from device memory at ptr {:#06x} with length @ {:#06x}", msg_ptr, msg_len_ptr);

      if (msg_len_ptr >= other_command_device::kMemorySize) {
        CORE_LOG_ERROR("[VM] Invalid string length pointer: {:#06x}", msg_len_ptr);
        device->write_flag_register(VM_INVALID_MEMORY_ACCESS);
        device->write_register_from_u64(vm_register::kReturnRegister, 0);
        return {};
      }

      uint64_t msg_len = device->current_program_data_as_u64(static_cast<uint16_t>(msg_len_ptr));
      CORE_LOG_DEBUG("[VM] String length read from device memory: {}", msg_len);

      if (msg_ptr >= other_command_device::kMemorySize ||
          msg_len > other_command_device::kMemorySize - msg_ptr) {
        CORE_LOG_ERROR("[VM] Invalid string pointer or length: ptr {:#06x}, len {}", msg_ptr, msg_len);
        device->write_flag_register(VM_INVALID_MEMORY_ACCESS);
        device->write_register_from_u64(vm_register::kReturnRegister, 0);
        return {};
      }

      /// make sure msg_len is reasonable
      if (msg_len > other_command_device::kMaxStringLen) {
        CORE_LOG_WARN("[VM] Log message length too large: {}", msg_len);
        CORE_LOG_WARN("[VM] string will be truncated to maximum length {}, string ptr: {:#06x}, actual string len: {}", other_command_device::kMaxStringLen, msg_ptr, msg_len);
        msg_len = other_command_device::kMaxStringLen;
      }

      const void* ptr = device->access_current_program_memory(static_cast<uint16_t>(msg_ptr));
      std::string result(static_cast<const char*>(ptr), static_cast<size_t>(msg_len));
      return result;
    }

    uint64_t write_device_bytes(other_command_device* device, uint64_t offset, std::span<const uint8_t> bytes, uint64_t capacity) {
      if (offset + bytes.size() > capacity) {
        CORE_LOG_ERROR("[VM] Attempt to write beyond device buffer capacity: offset {} + size {} > capacity {}", offset, bytes.size(), capacity);
        device->write_flag_register(VM_OUT_OF_MEMORY);
        device->stopped = true;
        vm::add_flag(device, other_command_device::VM_ERROR);
        return offset;  // do not write anything if it exceeds capacity
      }

      device->write_current_program_memory(static_cast<uint16_t>(offset), bytes.data(), bytes.size());
      return offset + bytes.size();
    }

  }  // namespace detail
}  // namespace other