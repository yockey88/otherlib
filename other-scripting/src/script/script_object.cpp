/**
 * \file script/script_object.cpp
 **/
#include "script/script_object.hpp"

#include "dotnet/native_string.hpp"
#include "script/scripting_environment.hpp"

namespace other {

  behavior_snapshot script_object::get_behavior_snapshot() const {
    if (dotnet_object == nullptr) {
      return {};
    }
    return dotnet_object->get_behavior_snapshot();
  }

}  // namespace other