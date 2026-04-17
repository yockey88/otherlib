/**
 * \file scripting/dotnet_bindings/scene_object_bindings.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_DOTNET_BINDINGS_SCENE_OBJECT_BINDINGS_HPP
#define OTHERLIB_SCRIPTING_DOTNET_BINDINGS_SCENE_OBJECT_BINDINGS_HPP

#include "core/defines.hpp"

#include "dotnet/types.hpp"

namespace other {
  namespace bindings {

    void native_get_object_id(void* object_ptr, natural_t* out_id);
    void native_validate_object_handle(natural_t scene_id, natural_t object_id, int32_t generation, nbool32* out_is_valid);

  }  // namespace bindings
}  // namespace other

#endif  // OTHERLIB_SCRIPTING_DOTNET_BINDINGS_SCENE_OBJECT_BINDINGS_HPP