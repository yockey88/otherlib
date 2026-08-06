/**
 * \file object/physics_joint_component.hpp
 **/
#ifndef OTHER_SCENE_OBJECT_PHYSICS_JOINT_COMPONENT_HPP
#define OTHER_SCENE_OBJECT_PHYSICS_JOINT_COMPONENT_HPP

#include <string>

#include "core/defines.hpp"
#include "serialization/reflection.hpp"

namespace other {

  /// a breakable rigid weld to another object's physics body; created when the scene plays,
  ///   resolved by NAME (the engine's restore-stable key). the seed of the construction model
  struct physics_joint_component {
    std::string target_object_name;
    /// sustained force that breaks the weld, newtons; 0 = unbreakable
    float break_force = 0.f;

    /// runtime (unreflected)
    integer_t joint_id = -1;
    bool broken = false;
  };

}  // namespace other

OTHER_REFLECT(
  other::physics_joint_component,
  field(target_object_name, other::attr::serializable("Attached To")),
  field(break_force, other::attr::serializable("Break Force")))

#endif  // OTHER_SCENE_OBJECT_PHYSICS_JOINT_COMPONENT_HPP
