/**
 * \file vm/devices/project_device.cpp
 **/
#include "vm/devices/project_device.hpp"

#include <filesystem>

#include "driver/driver.hpp"
#include "driver/systems/project_system.hpp"
#include "vm/device_utils.hpp"
#include "vm/other_device.hpp"
#include "vm/vm_error.hpp"

namespace other {
  namespace {

    constexpr type_descriptor kLoadedParams[] = {
      {}
    };
    constexpr type_descriptor kLoadParams[] = {
      { VM_TYPE_PTR, "directory-ptr" },
      { VM_TYPE_U64, "directory-len" },
    };
    constexpr type_descriptor kUnloadParams[] = {
      {}
    };

    constexpr type_descriptor kVoidRet = { VM_TYPE_VOID, "void" };
    constexpr type_descriptor kU64Ret = { VM_TYPE_U64, "value" };

    constexpr function_descriptor kFunctions[] = {
      { project_device::LOADED, "loaded", kLoadedParams, kU64Ret },
      { project_device::LOAD, "load", kLoadParams, kVoidRet },
      { project_device::UNLOAD, "unload", kUnloadParams, kVoidRet }
    };

  }  // namespace

  std::span<const function_descriptor> project_device::functions() const {
    return kFunctions;
  }

  void project_device::dispatch(uint8_t function_id, other_command_device* device) {
    OTHER_ASSERT(device != nullptr, "device must not be null");
    switch (function_id) {
      case LOADED: {
        auto& kernel = device->host_driver->get_kernel();
        auto& projects = kernel.get_core_system<project_system>();
        device->write_return_register(static_cast<uint64_t>(projects.is_project_loaded()));
      } break;
      case LOAD: {
        std::string dir = detail::read_device_string(device, vm_register_idx::VM_R0, vm_register_idx::VM_R1);
        if (device->read_flag_register() != VM_OK) {
          return;
        }

        filepath path = dir;
        if (!std::filesystem::exists(path)) {
          path = std::filesystem::absolute(path);
          if (!std::filesystem::exists(path)) {
            device->write_flag_register(vm_error::VM_BAD_ARGUMENT);
            device->write_return_register(0);
            return;
          }
        }

        // At this point, path is a valid directory
        auto& kernel = device->host_driver->get_kernel();
        auto& projects = kernel.get_core_system<project_system>();

        project_event_data event_data{
          .type = "load",
          .project_path = path.string(),
        };
        projects.handle_project_event(&kernel, event_data);
      } break;
      case UNLOAD:
        // Implement the UNLOAD function
        break;
      default:
        OTHER_ASSERT(false, "Unknown function id");
        break;
    }
  }

}  // namespace other