/**
 * \file scripting/dotnet_bindings/component_bindings.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_DOTNET_BINDINGS_COMPONENT_BINDINGS_HPP
#define OTHERLIB_SCRIPTING_DOTNET_BINDINGS_COMPONENT_BINDINGS_HPP

#include "core/defines.hpp"

namespace other {
  namespace bindings {

    void native_transform_get_position(natural_t id, float* out_x, float* out_y, float* out_z);
    void native_transform_set_position(natural_t id, float x, float y, float z);
    void native_transform_get_rotation(natural_t id, float* out_x, float* out_y, float* out_z, float* out_w);
    void native_transform_set_rotation(natural_t id, float x, float y, float z, float w);
    void native_transform_get_scale(natural_t id, float* out_x, float* out_y, float* out_z);
    void native_transform_set_scale(natural_t id, float x, float y, float z);
    void native_transform_get_world_matrix(natural_t id, float* out_matrix);

  }  // namespace bindings
}  // namespace other

#endif  // OTHERLIB_SCRIPTING_DOTNET_BINDINGS_COMPONENT_BINDINGS_HPP