/**
 * \file object/script_component.hpp
 **/
#ifndef OTHER_SCENE_OBJECT_SCRIPT_COMPONENT_HPP
#define OTHER_SCENE_OBJECT_SCRIPT_COMPONENT_HPP

#include <cstdint>

#include <glm/glm.hpp>

#include "serialization/reflection.hpp"

#include "script/script_object.hpp"

#include "asset/asset.hpp"

namespace other {

  struct scene_object;

  struct script_component {
    scene_object* object = nullptr;
    integer_t script_object_id = -1;

    /// behavior bookkeeping lives on the script_object (behavior_handles) in the
    ///  scripting_environment, keyed by script_object_id

    void fixed_update(double delta_time);
    void update(double delta_time);
    void late_update(double delta_time);

    /// collision/trigger dispatch: method is the managed entry point name
    ///  ("CollisionEnter"/"CollisionExit"/"TriggerEnter"/"TriggerExit")
    void dispatch_physics_event(const char* method, natural_t other_id, const glm::vec3& point, const glm::vec3& normal);
    void dispatch_joint_break(float force);
    /// per-frame draw hook, dispatched whether or not the scene is playing
    void render_update(double delta_time);

    void scene_start();
    void scene_stop();
    /// end-of-disable hook for play-stop restores: the managed instance survives the
    ///  native rebuild, so only its native binding is reset here (no Remove fires)
    void reset_dotnet_binding();

    void add_behavior(const std::string_view behavior_type_name);
    void remove_behavior(const std::string_view behavior_type_name);
    void remove_all_behaviors();

    script_component() = default;
    script_component(const script_component& other) {
      this->object = other.object;
      this->script_object_id = other.script_object_id;
    }
    script_component(scene_object* obj) : object(obj) {
      OTHER_ASSERT(object != nullptr, "Script component initialized with null scene object.");
    }
  };

  struct script_component_lua_proxy {
    script_component* comp = nullptr;
  };

}  // namespace other

// clang-format off
OTHER_REFLECT(
  other::script_component,
  field(script_object_id, other::attr::serializable("Script Object ID"), 
                          other::attr::asset_identifier_field(other::asset::SCRIPT))
);
// clang-format on

#endif  // OTHER_SCENE_OBJECT_SCRIPT_COMPONENT_HPP
