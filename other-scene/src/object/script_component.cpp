/**
 * file object/script_component.cpp
 **/
#include "object/script_component.hpp"

#include "script/scripting_environment.hpp"

namespace other {

  void script_component::fixed_update(double delta_time) {
    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "Scripting environment is not initialized.");

    script_object* script_obj = env->get_object(script_object_id);
    OTHER_ASSERT(script_obj != nullptr, "Script object with ID {} not found in scripting environment.", script_object_id);

    if (auto* dn_obj = script_obj->dotnet_object; dn_obj != nullptr) {
      // dn_obj->invoke<>("FixedUpdate", delta_time);
    }

    if (auto* py_obj = script_obj->python_object; py_obj != nullptr) {
      // py_obj->invoke("FixedUpdate", delta_time);
    }
  }

  void script_component::update(double delta_time) {
  }

  void script_component::late_update(double delta_time) {
  }

}  // namespace other
