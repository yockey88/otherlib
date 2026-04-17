/**
 * \file scripting/dotnet_bindings/component_bindings.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_DOTNET_BINDINGS_COMPONENT_BINDINGS_HPP
#define OTHERLIB_SCRIPTING_DOTNET_BINDINGS_COMPONENT_BINDINGS_HPP

#include "core/defines.hpp"

namespace other {
  namespace bindings {

    void native_transform_get_world_matrix(natural_t id, float* out_matrix);

  }  // namespace bindings
}  // namespace other

#endif  // OTHERLIB_SCRIPTING_DOTNET_BINDINGS_COMPONENT_BINDINGS_HPP