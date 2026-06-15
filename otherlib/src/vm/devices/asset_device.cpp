/**
 * \file vm/devices/asset_device.cpp
 **/
#include "vm/devices/asset_device.hpp"

#include "driver/driver.hpp"
#include "vm/device_utils.hpp"
#include "vm/vm.hpp"
#include "vm/vm_error.hpp"
#include "vm/vm_function.hpp"

namespace other {
  namespace {

    constexpr type_descriptor kLoadAssetParams[] = {
      { VM_TYPE_PTR, "path-ptr" },
      { VM_TYPE_U64, "path-len" },
    };
    constexpr type_descriptor kGetAssetStateParams[] = {
      { VM_TYPE_U64, "asset-id" },
    };
    constexpr type_descriptor kGetAssetIdParams[] = {
      { VM_TYPE_PTR, "path-ptr" },
      { VM_TYPE_U64, "path-len" },
    };
    constexpr type_descriptor kUnloadAssetParams[] = {
      { VM_TYPE_U64, "asset-id" },
    };
    constexpr type_descriptor kReloadAssetParams[] = {
      { VM_TYPE_U64, "asset-id" },
    };
    constexpr type_descriptor kVoidRet = { VM_TYPE_VOID, "void" };
    constexpr type_descriptor kU64Ret = { VM_TYPE_U64, "asset-id" };

    constexpr function_descriptor kFunctions[] = {
      { asset_device::LOAD_ASSET, "load_asset", kLoadAssetParams, kU64Ret, VM_SE_WRITES_HOST | VM_SE_MAY_BLOCK },
      { asset_device::GET_ASSET_STATE, "get_asset_state", kGetAssetStateParams, kU64Ret, VM_SE_READS_HOST },
      { asset_device::GET_ASSET_ID, "get_asset_id", kGetAssetIdParams, kU64Ret, VM_SE_READS_HOST },
      { asset_device::UNLOAD_ASSET, "unload_asset", kUnloadAssetParams, kVoidRet, VM_SE_WRITES_HOST | VM_SE_MAY_BLOCK },
      { asset_device::RELOAD_ASSET, "reload_asset", kReloadAssetParams, kVoidRet, VM_SE_WRITES_HOST | VM_SE_MAY_BLOCK },
    };

  }  // namespace

  std::span<const function_descriptor> asset_device::functions() const {
    return kFunctions;
  }

  void asset_device::dispatch(uint8_t function_id, other_command_device* device) {
    OTHER_ASSERT(device != nullptr, "device must not be null");
    switch (function_id) {
      case LOAD_ASSET: {
        std::string path = detail::read_device_string(device, vm_register_idx::VM_R0, vm_register_idx::VM_R1);
        if (device->read_flag_register() != VM_OK) {
          break;
        }

        natural_t asset_id = device->host_driver->begin_asset_load(path);
        device->write_return_register(asset_id);
      } break;
      case GET_ASSET_STATE: {
        natural_t asset_id = device->read_register_as_u64(vm_register_idx::VM_R0);
        natural_t asset_state = device->host_driver->get_asset_state(asset_id);
        device->write_return_register(asset_state);
      } break;
      case GET_ASSET_ID: {
        std::string path = detail::read_device_string(device, vm_register_idx::VM_R0, vm_register_idx::VM_R1);
        if (device->read_flag_register() != VM_OK) {
          break;
        }
        natural_t asset_id = device->host_driver->get_asset_id_from_path(path);
        device->write_return_register(asset_id);
      } break;
      case UNLOAD_ASSET: {
        natural_t asset_id = device->read_register_as_u64(vm_register_idx::VM_R0);
        device->host_driver->begin_asset_unload(asset_id);
      } break;
      case RELOAD_ASSET: {
        CORE_LOG_WARN("Reload asset is not yet implemented");
      } break;
      default:
        OTHER_ASSERT(false, "unknown function id");
        break;
    }
  }

}  // namespace other