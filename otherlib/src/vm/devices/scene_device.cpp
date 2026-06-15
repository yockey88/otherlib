/**
 * \file vm/devices/scene_device.cpp
 **/
#include "vm/devices/scene_device.hpp"

#include "driver/driver.hpp"
#include "vm/other_device.hpp"

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
    constexpr type_descriptor kGetSceneIdParams[] = {
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
      { scene_device::GET_SCENE_ID, "get_scene_id", kGetSceneIdParams, kU64Ret, VM_SE_PURE },
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
      } break;
      case scene_device::GET_ACTIVE_SCENE_ID: {
      } break;
      case scene_device::FIND_OBJECT: {
      } break;
      case scene_device::CREATE_OBJECT: {
      } break;
      case scene_device::DESTROY_OBJECT: {
      } break;
      case scene_device::SET_OBJECT_VISIBLE: {
      } break;
      case scene_device::GET_SCENE_ID: {
      } break;
      default:
        OTHER_ASSERT(false, "Unknown function id");
    }
  }

}  // namespace other