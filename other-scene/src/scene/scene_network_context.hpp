/**
 * \file scene/scene_network_context.hpp
 **/
#ifndef OTHER_SCENE_SCENE_SCENE_NETWORK_CONTEXT_HPP
#define OTHER_SCENE_SCENE_SCENE_NETWORK_CONTEXT_HPP

#include "core/defines.hpp"

#include "object/transform.hpp"

namespace other {

  struct net_object_entry {
    natural_t net_id = 0;
    /// runtime scene id — remints on snapshot restore, entries are rebuilt then
    natural_t object_id = 0;
    uint16_t owner_peer = 0;  // 0 = host
    bool replicate_transform = true;
    /// host bookkeeping
    bool spawn_sent = false;
    /// host dirty-check cache (local TRS)
    transform last_sent;
  };

  enum class replication_role : uint8_t {
    NONE,
    AUTHORITY,
    REPLICA,
  };

  /// the scene's net-identity registry — pure state, zero networking includes.
  ///  a scene member rather than a component: nothing here is per-object-authored
  ///  or serialized (authored intent lives on network_component)
  class scene_network_context {
   public:
    replication_role role = replication_role::NONE;

    /// AUTHORITY: assigns the next net id; monotonic, never recycled
    natural_t register_object(natural_t object_id, uint16_t owner_peer);
    /// REPLICA: adopt an authority-assigned id from SPAWN/snapshot
    void adopt(natural_t net_id, natural_t object_id, uint16_t owner_peer);
    void forget(natural_t net_id);
    void clear();

    opt<natural_t> object_of(natural_t net_id) const;
    opt<natural_t> net_of(natural_t object_id) const;
    std::span<net_object_entry> entries() { return objects; }

   private:
    natural_t next_net_id = 1;
    ostd::vector<net_object_entry> objects;
    ostd::map<natural_t, size_t> by_net_id;
    ostd::map<natural_t, size_t> by_object_id;

    void reindex();
  };

}  // namespace other

#endif  // OTHER_SCENE_SCENE_SCENE_NETWORK_CONTEXT_HPP
