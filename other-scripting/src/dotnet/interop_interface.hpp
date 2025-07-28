/**
 * \file dotnet/interop_interface.hpp
 **/
#ifndef OTHER_SCRIPTING_DOTNET_INTEROP_INTERFACE_HPP
#define OTHER_SCRIPTING_DOTNET_INTEROP_INTERFACE_HPP

#include "core/defines.hpp"

#include "dotnet/dotnet_assembly.hpp"
#include "dotnet/native_string.hpp"
#include "dotnet/types.hpp"

namespace other {

  /// AssemblyLoader
  using create_assembly_load_context = int32_t (*)(native_string);
  using unload_assembly_load_context = void (*)(int32_t);
  using load_managed_assembly = int32_t (*)(int32_t, native_string);
  using get_last_load_status = assembly_load_status (*)();
  using get_assembly_name = native_string (*)(int32_t);

  /// NativeFunctionManager
  using register_internal_call = void (*)(native_string, void*);

  /// InteropInterface
  using get_net_core_types = void (*)(int32_t*, int32_t*);
  using get_assembly_types = void (*)(int32_t, int32_t*, int32_t*);
  using get_type_id = void (*)(native_string, int32_t*);
  using get_full_type_name = native_string (*)(int32_t);
  using get_asm_qualified_name = native_string (*)(int32_t);
  using get_base_type = void (*)(int32_t, int32_t*);
  using get_type_size = int32_t (*)(int32_t);
  using is_type_derived_from = nbool32 (*)(int32_t, int32_t);
  using is_assignable_to = nbool32 (*)(int32_t, int32_t);
  using is_assignable_from = nbool32 (*)(int32_t, int32_t);
  using is_type_sz_array = nbool32 (*)(int32_t);
  using get_element_type = void (*)(int32_t, int32_t*);
  using get_type_methods = void (*)(int32_t, int32_t*, int32_t*);
  using get_type_fields = void (*)(int32_t, int32_t*, int32_t*);
  using get_type_properties = void (*)(int32_t, int32_t*, int32_t*);
  using has_attribute = nbool32 (*)(int32_t, int32_t);
  using get_attributes = void (*)(int32_t, int32_t*, int32_t*);
  using get_type_managed_type = managed_type (*)(int32_t);

}  // namespace other

#endif  // OTHER_SCRIPTING_DOTNET_INTEROP_INTERFACE_HPP