/**
 * \file dotnet/interop_interface.hpp
 **/
#ifndef OTHER_SCRIPTING_DOTNET_INTEROP_INTERFACE_HPP
#define OTHER_SCRIPTING_DOTNET_INTEROP_INTERFACE_HPP

#include <cstdint>

#include "core/defines.hpp"

#include "dotnet/dotnet_assembly.hpp"
#include "dotnet/garbage_collector.hpp"
#include "dotnet/native_string.hpp"
#include "dotnet/types.hpp"

namespace other {

  /// AssemblyLoader
  using create_assembly_load_context = int32_t (*)(native_string);
  using unload_assembly_load_context = void (*)(int32_t);
  using load_managed_assembly = int32_t (*)(int32_t, native_string);
  using unload_managed_assembly = void (*)(int32_t);
  using get_last_load_status = assembly_load_status (*)();
  using get_assembly_name = native_string (*)(int32_t);

  /// NativeFunctionManager
  using discover_binding_points = void (*)();
  using bind_native_function = void (*)(native_string, void*);
  using register_internal_call = void (*)(native_string, void*);
  using validate_binding_points = nbool32 (*)();

  /// NativeObjectManager
  using attach_native_object = void (*)(int64_t, void*, native_string);
  using detach_native_object = void (*)(int64_t, void*);

  /// TypeInterface
  using get_net_core_types = void (*)(int32_t*, int32_t*);
  using get_type_id = void (*)(native_string, int32_t*);
  using get_type_name = native_string (*)(int32_t);
  using get_base_type = void (*)(int32_t, int32_t*);
  using get_type_size = int32_t (*)(int32_t);
  using check_type_characteristic = nbool32 (*)(int32_t, int32_t);
  using is_type_sz_array = nbool32 (*)(int32_t);
  using get_element_type = void (*)(int32_t, int32_t*);
  using get_type_information = void (*)(int32_t, int32_t*, int32_t*);
  using get_type_managed_type = managed_type (*)(int32_t);

  /// method
  using get_method_name = native_string (*)(int32_t);
  using get_method_return_type = void (*)(int32_t, int32_t*);
  using get_method_accessibility = type_accessibility (*)(int32_t);

  /// field
  using field_property_checker = nbool32 (*)(int32_t, native_string);
  using get_field_name = native_string (*)(int32_t);
  using get_field_type = void (*)(int32_t, int32_t*);
  using get_field_value_type = void (*)(int32_t, uint8_t*);
  using get_field_accessibility = type_accessibility (*)(int32_t);
  using get_field_attributes = void (*)(int32_t, int32_t*, int32_t*);
  using get_default_value = void (*)(int32_t, void*);

  /// property
  using get_property_name = native_string (*)(int32_t);
  using get_property_type = void (*)(int32_t, int32_t*);
  using get_property_attributes = void (*)(int32_t, int32_t*, int32_t*);

  /// attribute
  using get_attribute_type = void (*)(int32_t, int32_t*);
  using get_managed_object_from_object = void (*)(int32_t, native_string, void*);

  /// ManagedObject
  using create_object = void* (*)(int32_t, nbool32, const void**, const managed_type*, int32_t);
  using destroy_object = void (*)(void*);
  using invoke_method = void (*)(void*, native_string, const void**, const managed_type*, int32_t);
  using invoke_method_ret = void (*)(void*, native_string, const void**, const managed_type*, int32_t, void*);
  using field_is_private_checker = nbool32 (*)(void*, native_string);
  using field_setter_getter = void (*)(void*, native_string, void*);
  using string_field_setter_getter = void (*)(void*, native_string, native_string*);
  using managed_strlen = size_t (*)(void*, native_string);

  /// GarbageCollector
  using collect_garbage = void (*)(int32_t, gc_mode, nbool32, nbool32);
  using wait_for_pending_finalizers = void (*)();

}  // namespace other

#endif  // OTHER_SCRIPTING_DOTNET_INTEROP_INTERFACE_HPP