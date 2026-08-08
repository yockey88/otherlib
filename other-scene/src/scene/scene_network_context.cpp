/**
 * \file scene/scene_network_context.cpp
 **/
#include "scene/scene_network_context.hpp"

#include "core/logger.hpp"

namespace other {

  natural_t scene_network_context::register_object(natural_t object_id, uint16_t owner_peer) {
    if (opt<natural_t> existing = net_of(object_id); existing.has_value()) {
      return *existing;
    }
    const natural_t net_id = next_net_id++;
    by_net_id[net_id] = objects.size();
    by_object_id[object_id] = objects.size();
    objects.push_back({ .net_id = net_id, .object_id = object_id, .owner_peer = owner_peer });
    return net_id;
  }

  void scene_network_context::adopt(natural_t net_id, natural_t object_id, uint16_t owner_peer) {
    if (by_net_id.contains(net_id)) {
      CORE_LOG_WARN("net id {} adopted twice; keeping the first mapping", net_id);
      return;
    }
    by_net_id[net_id] = objects.size();
    by_object_id[object_id] = objects.size();
    objects.push_back({ .net_id = net_id, .object_id = object_id, .owner_peer = owner_peer, .spawn_sent = true });
  }

  void scene_network_context::forget(natural_t net_id) {
    auto itr = by_net_id.find(net_id);
    if (itr == by_net_id.end()) {
      return;
    }
    objects.erase(objects.begin() + itr->second);
    reindex();
  }

  void scene_network_context::clear() {
    objects.clear();
    by_net_id.clear();
    by_object_id.clear();
  }

  opt<natural_t> scene_network_context::object_of(natural_t net_id) const {
    auto itr = by_net_id.find(net_id);
    return itr != by_net_id.end() ? opt<natural_t>{ objects[itr->second].object_id } : std::nullopt;
  }

  opt<natural_t> scene_network_context::net_of(natural_t object_id) const {
    auto itr = by_object_id.find(object_id);
    return itr != by_object_id.end() ? opt<natural_t>{ objects[itr->second].net_id } : std::nullopt;
  }

  void scene_network_context::reindex() {
    by_net_id.clear();
    by_object_id.clear();
    for (size_t i = 0; i < objects.size(); ++i) {
      by_net_id[objects[i].net_id] = i;
      by_object_id[objects[i].object_id] = i;
    }
  }

}  // namespace other
