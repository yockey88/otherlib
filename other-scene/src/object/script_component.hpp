/**
 * \file object/script_component.hpp
 **/
#ifndef OTHER_SCENE_OBJECT_SCRIPT_COMPONENT_HPP
#define OTHER_SCENE_OBJECT_SCRIPT_COMPONENT_HPP

#include <cstdint>

#include "serialization/reflection.hpp"

#include "script/script_object.hpp"

#include "asset/asset.hpp"

namespace other {

  struct scene_object;

  struct script_component {
    scene_object* object = nullptr;
    integer_t script_object_id = -1;

    /// behavior script_object IDs managed by the scripting_environment.
    /// each behavior is a separate script_object with its own dotnet_object,
    /// but is also linked to the parent SceneObject's behavior list in C#.
    ostd::vector<integer_t> behavior_ids;

    void fixed_update(double delta_time);
    void update(double delta_time);
    void late_update(double delta_time);
    /// per-frame draw hook, dispatched whether or not the scene is playing
    void render_update(double delta_time);

    void scene_start();
    void scene_stop();

    void add_behavior(const std::string_view behavior_type_name);
    void remove_behavior(const std::string_view behavior_type_name);
    void remove_all_behaviors();

    script_component() = default;
    script_component(const script_component& other) {
      this->object = other.object;
      this->script_object_id = other.script_object_id;
      this->behavior_ids = other.behavior_ids;
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
