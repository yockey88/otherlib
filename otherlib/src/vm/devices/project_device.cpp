/**
 * \file vm/devices/project_device.cpp
 **/
#include "vm/devices/project_device.hpp"

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
      {}
    };

  }  // namespace

  std::span<const function_descriptor> project_device::functions() const {
    return kFunctions;
  }

  void project_device::dispatch(uint8_t function_id, other_command_device* device) {
    OTHER_ASSERT(device != nullptr, "device must not be null");
    CORE_LOG_WARN("No project functions implemented yet");
  }

}  // namespace other