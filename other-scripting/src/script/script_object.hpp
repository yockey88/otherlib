/**
 * \file script/script_object.hpp
 **/
#ifndef OTHER_SCRIPTING_SCRIPT_SCRIPT_OBJECT_HPPP
#define OTHER_SCRIPTING_SCRIPT_SCRIPT_OBJECT_HPPP

#include <refl/refl.hpp>

#include "core/defines.hpp"

#include "dotnet/behavior_descriptor.hpp"

namespace other {

  class dotnet_object;

  struct script_object {
    script_object() = default;
    ~script_object() = default;

    std::string name;
    integer_t id = -1;

    dotnet_object* dotnet_object = nullptr;

    /// set when this object was registered with the C# NativeObjectManager (scene objects
    ///  are, behaviors are not) so detach only unregisters what was actually attached
    bool dotnet_native_registered = false;

    /// behavior dotnet_objects attached to this script_object: managed user-script instances
    ///  added via attach_dotnet_behavior; also mirrored in the parent OtherObject's behavior list
    struct behavior_handle {
      std::string type_name;
      integer_t script_object_id = -1;  ///< the script_object ID for this behavior in the pool
    };
    ostd::vector<behavior_handle> behavior_handles;

    behavior_snapshot get_behavior_snapshot() const;
  };

}  // namespace other

#endif  // OTHER_SCRIPTING_SCRIPT_SCRIPT_OBJECT_HPPP