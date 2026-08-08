/**
 * \file script/script_object.cpp
 **/
#include "script/script_object.hpp"

#include "core/profiler.hpp"

#include "dotnet/dotnet_object.hpp"
#include "dotnet/native_string.hpp"
#include "script/scripting_environment.hpp"

namespace other {

  /// built from each behavior's own dotnet_object — the managed side exposes no parent-indexed
  ///  field surface; each attached behavior's script_object already carries its own field list
  behavior_snapshot script_object::get_behavior_snapshot() const {
    PROFILE_SECTION("script_object::get_behavior_snapshot");
    if (dotnet_object == nullptr) {
      return {};
    }

    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "Scripting environment is not initialized.");

    behavior_snapshot snapshot{};
    snapshot.valid = true;

    for (int32_t bi = 0; bi < (int32_t)behavior_handles.size(); ++bi) {
      const behavior_handle& handle = behavior_handles[bi];
      /// -1 = invalidated mid assembly-refresh; the slot reattaches later
      if (handle.script_object_id < 0) {
        continue;
      }

      script_object* behavior = env->get_object(handle.script_object_id);
      if (behavior == nullptr || behavior->dotnet_object == nullptr || behavior->dotnet_object->dn_type == nullptr) {
        continue;
      }

      behavior_descriptor desc{};
      desc.behavior_index = bi;
      desc.script_object_id = handle.script_object_id;
      desc.full_type_name = behavior->dotnet_object->get_type_name();
      desc.display_name = behavior->dotnet_object->dn_type->class_name();

      int32_t fi = 0;
      for (const dotnet_field& f : behavior->dotnet_object->dn_type->get_fields()) {
        /// plain data fields only, matching the eager-load policy: property getters can
        ///  run arbitrary code and user types have no flat representation
        if (f.is_property() || f.name().ends_with("k__BackingField")) {
          continue;
        }
        const value_type ft = f.get_type();
        if (ft == value_type::USER_TYPE || ft == value_type::EMPTY_TYPE) {
          continue;
        }

        behavior_field_descriptor fd{};
        fd.field_name = f.name();
        fd.display_name = f.name();
        fd.type = ft;
        fd.field_index = fi++;
        desc.fields.push_back(std::move(fd));
      }

      snapshot.behaviors.push_back(std::move(desc));
    }

    return snapshot;
  }

}  // namespace other
