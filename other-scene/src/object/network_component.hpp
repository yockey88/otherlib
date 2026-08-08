/**
 * \file object/network_component.hpp
 **/
#ifndef OTHER_SCENE_OBJECT_NETWORK_COMPONENT_HPP
#define OTHER_SCENE_OBJECT_NETWORK_COMPONENT_HPP

#include "core/defines.hpp"
#include "serialization/reflection.hpp"

namespace other {

  /// authored intent: "this is a networked thing". runtime net_id lives in the
  ///  scene's network context — nothing session-scoped serializes.
  ///  (u32/bool fields keep it scriptable through the generic field ABI)
  struct network_component {
    uint32_t owner_peer = 0;  // 0 = host
    bool replicate_transform = true;
    bool despawn_on_owner_leave = true;
  };

}  // namespace other

OTHER_REFLECT(
  other::network_component,
  field(owner_peer, other::attr::serializable("Owner Peer")),
  field(replicate_transform, other::attr::serializable("Replicate Transform")),
  field(despawn_on_owner_leave, other::attr::serializable("Despawn On Owner Leave")))

#endif  // OTHER_SCENE_OBJECT_NETWORK_COMPONENT_HPP
