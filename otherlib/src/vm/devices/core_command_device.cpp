/**
 * \file vm/devices/core_command_device.cpp
 **/
#include "vm/devices/core_command_device.hpp"

#include "core/logger.hpp"

#include "driver/driver.hpp"
#include "vm/device_utils.hpp"
#include "vm/other_device.hpp"
#include "vm/register.hpp"
#include "vm/vm.hpp"
#include "vm/vm_error.hpp"

namespace other {
  namespace {

    constexpr type_descriptor kLogParams[] = {
      { VM_TYPE_PTR, "msg-ptr" },
      { VM_TYPE_U64, "msg-len" },
      { VM_TYPE_U64, "level" },
    };
    constexpr type_descriptor kLogU64Params[] = {
      { VM_TYPE_U64, "value" },
      { VM_TYPE_U64, "level" },
    };
    constexpr type_descriptor kErrorNameParams[] = {
      { VM_TYPE_U64, "error-code" },
      { VM_TYPE_PTR, "dst-ptr" },
      { VM_TYPE_U64, "dst-cap" },
    };
    constexpr type_descriptor kVoidRet = { VM_TYPE_VOID, "void" };
    constexpr type_descriptor kU64Ret = { VM_TYPE_U64, "value" };

    constexpr function_descriptor kFunctions[] = {
      { core_command_device::LOG, "log", kLogParams, kVoidRet, VM_SE_WRITES_HOST },
      { core_command_device::LOG_U64, "log_u64", kLogU64Params, kVoidRet, VM_SE_WRITES_HOST },
      { core_command_device::INIT_TIME, "init_time", {}, kU64Ret, VM_SE_READS_HOST },
      { core_command_device::TIME_SINCE_INIT, "time_since_init", {}, kU64Ret, VM_SE_READS_HOST },
      { core_command_device::TIME_SINCE_EPOCH, "epoch_time", {}, kU64Ret, VM_SE_READS_HOST },
      { core_command_device::RANDOM, "random", {}, kU64Ret, VM_SE_PURE },
      { core_command_device::ERROR_NAME, "error_name", kErrorNameParams, kU64Ret, VM_SE_PURE },
    };

    constexpr logger::level to_log_level(uint64_t level) {
      constexpr uint64_t kMaxLevel = static_cast<uint64_t>(logger::level::CRITICAL);
      return static_cast<logger::level>(std::min(level, kMaxLevel));
    }

  }  // namespace

  std::span<const function_descriptor> core_command_device::functions() const {
    return kFunctions;
  }

  void core_command_device::dispatch(uint8_t function_id, other_command_device* device) {
    OTHER_ASSERT(device != nullptr, "Device cannot be null for dispatch");
    CORE_LOG_DEBUG("[VM] Core device dispatch called with function ID {:#04x}", function_id);
    switch (function_id) {
      case LOG: {
        std::string msg = detail::read_device_string(device, vm_register_idx::VM_R0, vm_register_idx::VM_R1);
        if (device->read_flag_register() != VM_OK) {
          CORE_LOG_WARN("[VM] Log message is empty");
          vm::add_flag(device, other_command_device::VM_ERROR);
          return;
        }

        uint64_t level = device->read_register_as_u64(vm_register_idx::VM_R2);
        CORE_LOG_MESSAGE(to_log_level(level), "[VM: CORE.LOG] {}", msg);
      } break;
      case LOG_U64: {
        uint64_t value = device->read_register_as_u64(vm_register_idx::VM_R0);
        uint64_t level = device->read_register_as_u64(vm_register_idx::VM_R1);

        if (device->read_flag_register() == 1) {
          CORE_LOG_MESSAGE(to_log_level(level), "[VM: CORE.LOG_U64] {}", value);
        } else {
          CORE_LOG_MESSAGE(to_log_level(level), "[VM: CORE.LOG_U64] {:#018x}", value);
        }
        device->write_flag_register(0);
      } break;

      case INIT_TIME: {
        device->write_register_from_u64(vm_register_idx::VM_RRETURN, device->device_init_time);
      } break;
      case TIME_SINCE_INIT: {
        device->write_register_from_u64(vm_register_idx::VM_RRETURN, device->time_since_init);
      } break;
      case TIME_SINCE_EPOCH: {
        device->write_register_from_u64(vm_register_idx::VM_RRETURN, device->epoch_time);
      } break;

      case RANDOM: {
        uint64_t random_value = device->get_random_byte();
        device->write_register_from_u64(vm_register_idx::VM_RRETURN, random_value);
      } break;

      case ERROR_NAME: {
        uint64_t error_code = device->read_register_as_u64(vm_register_idx::VM_R0);
        uint64_t dst_ptr = device->read_register_as_u64(vm_register_idx::VM_R1);
        uint64_t dst_cap = device->read_register_as_u64(vm_register_idx::VM_R2);

        if (error_code >= std::numeric_limits<uint16_t>::max()) {
          device->write_flag_register(VM_INVALID_MEMORY_ACCESS);
          device->write_register_from_u64(vm_register::kReturnRegister, 0);
          vm::add_flag(device, other_command_device::VM_ERROR);
          return;
        }

        std::string error_name = get_vm_error_name(static_cast<other::vm_error>(error_code));
        size_t bytes_to_write = std::min(error_name.size(), static_cast<size_t>(dst_cap));
        CORE_LOG_TRACE("Writing '{}' ({} bytes) to memory at address {:#010x} with capacity {}", error_name, bytes_to_write, dst_ptr, dst_cap);
        if (bytes_to_write > 0) {
          device->write_current_program_memory(dst_ptr, reinterpret_cast<const uint8_t*>(error_name.data()), bytes_to_write);
        }
        device->write_register_from_u64(vm_register_idx::VM_RF, bytes_to_write);
      } break;
      default:
        OTHER_ASSERT(false, "Invalid function ID for core_device: {:#04x}", function_id);
    }
  }

}  // namespace other