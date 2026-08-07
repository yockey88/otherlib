/**
 * \file peer_mesh/network.cpp
 **/
#include "peer_mesh/network.hpp"

#include <algorithm>

namespace other {

  const link_record* network::link(natural_t link_id) const {
    auto itr = std::ranges::find(records, link_id, &link_record::link_id);
    return itr != records.end() ? &*itr : nullptr;
  }

  const link_record* network::link_between(node_id local, node_id remote) const {
    for (const link_record& record : records) {
      if (record.local == local && record.remote == remote && record.state == link_state::UP) {
        return &record;
      }
    }
    return nullptr;
  }

  link_record* network::mutable_link(natural_t link_id) {
    auto itr = std::ranges::find(records, link_id, &link_record::link_id);
    return itr != records.end() ? &*itr : nullptr;
  }

  network::link_runtime* network::runtime(natural_t link_id) {
    auto itr = runtimes.find(link_id);
    return itr != runtimes.end() ? &itr->second : nullptr;
  }

  natural_t network::link_for_conn(size_t transport_index, natural_t conn_id) const {
    auto itr = conn_links.find({ transport_index, conn_id });
    return itr != conn_links.end() ? itr->second : 0;
  }

  link_record& network::adopt(size_t transport_index, natural_t conn_id, node_id local, link_state initial,
                              const link_caps& caps, bool stream, uint32_t max_frame_size, microseconds now) {
    const natural_t id = next_link_id++;

    link_record record;
    record.link_id = id;
    record.local = local;
    record.connection_id = conn_id;
    record.state = initial;
    record.caps = caps;
    records.push_back(std::move(record));

    link_runtime rt;
    rt.reader = frame_reader{ max_frame_size };
    rt.stream = stream;
    rt.transport_index = transport_index;
    rt.opened_at = now;
    rt.last_rx = now;
    rt.last_tx = now;
    runtimes.emplace(id, std::move(rt));

    conn_links[{ transport_index, conn_id }] = id;
    return records.back();
  }

  void network::drop(natural_t link_id) {
    auto itr = std::ranges::find(records, link_id, &link_record::link_id);
    if (itr == records.end()) {
      return;
    }
    if (auto rt = runtimes.find(link_id); rt != runtimes.end()) {
      conn_links.erase({ rt->second.transport_index, itr->connection_id });
      runtimes.erase(rt);
    }
    records.erase(itr);
  }

}  // namespace other
