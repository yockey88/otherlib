/**
 * \file vm/devices/scene_device.cpp
 **/
#include "vm/devices/scene_device.hpp"

#include <filesystem>

#include "driver/driver.hpp"
#include "vm/device_utils.hpp"
#include "vm/other_device.hpp"
#include "vm/vm_error.hpp"

namespace other {
  namespace {

    constexpr type_descriptor kLoadSceneParams[] = {
      { VM_TYPE_PTR, "path-ptr" },
      { VM_TYPE_U64, "path-len" },
    };
    constexpr type_descriptor kGetActiveSceneIdParams[] = {
      { VM_TYPE_VOID, "void" }
    };
    constexpr type_descriptor kObjectManipParams[] = {
      { VM_TYPE_PTR, "name-ptr" },
      { VM_TYPE_U64, "name-len" },
    };
    constexpr type_descriptor kGetSceneIdFromNameParams[] = {
      { VM_TYPE_VOID, "name-ptr" },
      { VM_TYPE_U64, "name-len" }
    };
    constexpr type_descriptor kVoidRet = { VM_TYPE_VOID, "void" };
    constexpr type_descriptor kU64Ret = { VM_TYPE_U64, "value" };

    constexpr function_descriptor kFunctions[] = {
      { scene_device::LOAD_SCENE, "load_scene", kLoadSceneParams, kU64Ret, VM_SE_WRITES_HOST | VM_SE_MAY_BLOCK },
      { scene_device::GET_ACTIVE_SCENE_ID, "get_active_scene_id", kGetActiveSceneIdParams, kU64Ret, VM_SE_PURE },
      { scene_device::FIND_OBJECT, "find_object", kObjectManipParams, kU64Ret, VM_SE_WRITES_HOST },
      { scene_device::CREATE_OBJECT, "create_object", kObjectManipParams, kU64Ret, VM_SE_WRITES_HOST },
      { scene_device::DESTROY_OBJECT, "destroy_object", kObjectManipParams, kU64Ret, VM_SE_WRITES_HOST },
      { scene_device::SET_OBJECT_VISIBLE, "set_object_visible", kObjectManipParams, kU64Ret, VM_SE_WRITES_HOST },
      { scene_device::GET_SCENE_ID, "get_scene_id", kGetSceneIdFromNameParams, kU64Ret, VM_SE_PURE },
    };

  }  // namespace

  std::span<const function_descriptor> scene_device::functions() const {
    return kFunctions;
  }

  void scene_device::dispatch(uint8_t function_id, other_command_device* device) {
    OTHER_ASSERT(device != nullptr, "device must not be null");
    CORE_LOG_WARN("No scene functions implemented yet");
    switch (function_id) {
      case scene_device::LOAD_SCENE: {
        std::string path = detail::read_device_string(device, vm_register_idx::VM_R0, vm_register_idx::VM_R1);
        if (device->read_flag_register() != VM_OK) {
          return;
        }

        auto& kernel = device->host_driver->get_kernel();
        kernel.get_core_system<scene_system>().add_scene_to_scene_graph(path);
      } break;
      case scene_device::GET_ACTIVE_SCENE_ID: {
        auto& kernel = device->host_driver->get_kernel();
        auto& scene_sys = kernel.get_core_system<scene_system>();
        auto* active_scene = scene_sys.get_active_scene();
        if (active_scene == nullptr) {
          device->write_return_register(0);
        } else {
          device->write_return_register(active_scene->id);
        }
      } break;
      case scene_device::FIND_OBJECT: {
        std::string object_name = detail::read_device_string(device, vm_register_idx::VM_R0, vm_register_idx::VM_R1);
        if (device->read_flag_register() != VM_OK) {
          return;
        }

        auto& kernel = device->host_driver->get_kernel();
        auto& scene_sys = kernel.get_core_system<scene_system>();
        auto* active_scene = scene_sys.get_active_scene();
        if (active_scene == nullptr) {
          device->write_return_register(0);
        } else {
          auto* obj = active_scene->find_object(object_name);
          if (obj == nullptr) {
            device->write_return_register(0);
          } else {
            device->write_return_register(obj->id);
          }
        }
      } break;
      case scene_device::CREATE_OBJECT: {
        std::string object_name = detail::read_device_string(device, vm_register_idx::VM_R0, vm_register_idx::VM_R1);
        if (device->read_flag_register() != VM_OK) {
          return;
        }

        auto& kernel = device->host_driver->get_kernel();
        auto& scene_sys = kernel.get_core_system<scene_system>();
        auto* active_scene = scene_sys.get_active_scene();
        if (active_scene == nullptr) {
          device->write_return_register(0);
        } else {
          scene_object& obj = active_scene->create_object(object_name);
          device->write_return_register(obj.id);
        }
      } break;
      case scene_device::DESTROY_OBJECT: {
        std::string object_name = detail::read_device_string(device, vm_register_idx::VM_R0, vm_register_idx::VM_R1);
        if (device->read_flag_register() != VM_OK) {
          return;
        }

        auto& kernel = device->host_driver->get_kernel();
        auto& scene_sys = kernel.get_core_system<scene_system>();
        auto* active_scene = scene_sys.get_active_scene();
        if (active_scene == nullptr) {
          device->write_return_register(0);
        } else {
          auto* obj = active_scene->find_object(object_name);
          if (obj == nullptr) {
            device->write_return_register(0);
          } else {
            active_scene->destroy_object(obj->id);
            device->write_return_register(1);
          }
        }
      } break;
      case scene_device::SET_OBJECT_VISIBLE: {
        std::string object_name = detail::read_device_string(device, vm_register_idx::VM_R0, vm_register_idx::VM_R1);
        if (device->read_flag_register() != VM_OK) {
          return;
        }

        bool visible = device->read_register_as<bool>(vm_register_idx::VM_R2);
        auto& kernel = device->host_driver->get_kernel();
        auto& scene_sys = kernel.get_core_system<scene_system>();
        auto* active_scene = scene_sys.get_active_scene();
        if (active_scene == nullptr) {
          device->write_return_register(0);
        } else {
          auto* obj = active_scene->find_object(object_name);
          if (obj == nullptr) {
            device->write_return_register(0);
          } else {
            obj->visible = visible;
            device->write_return_register(1);
          }
        }
      } break;
      case scene_device::GET_SCENE_ID: {
        std::string scene_name = detail::read_device_string(device, vm_register_idx::VM_R0, vm_register_idx::VM_R1);
        if (device->read_flag_register() != VM_OK) {
          return;
        }

        auto& kernel = device->host_driver->get_kernel();
        auto& scene_sys = kernel.get_core_system<scene_system>();
        auto& scene_graph = scene_sys.get_scene_graph();
        auto* scene = scene_graph.find_scene(scene_name);
        if (scene == nullptr) {
          device->write_return_register(0);
        } else {
          device->write_return_register(scene->id);
        }
      } break;
      default:
        OTHER_ASSERT(false, "Unknown function id");
    }
  }

}  // namespace other