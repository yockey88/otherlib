/**
 * \file peer_mesh/network.hpp
 **/
#ifndef OTHER_NETWORK_PEER_MESH_NETWORK_HPP
#define OTHER_NETWORK_PEER_MESH_NETWORK_HPP

#include "core/defines.hpp"
#include "core/time.hpp"

#include "network/frame.hpp"
#include "peer_mesh/link.hpp"
#include "peer_mesh/node_id.hpp"

namespace other {

  class peer_mesh;

  /// a collection of links — storage + rx plumbing, never policy. links may ride different
  ///  transports; framing happens here (per link). the managing mesh drives lifecycle privately
  class network {
   public:
    /// live links only; view invalidated by link churn
    std::span<const link_record> links() const { return records; }
    const link_record* link(natural_t link_id) const;
    const link_record* link_between(node_id local, node_id remote) const;
    size_t link_count() const { return records.size(); }

   private:
    friend class peer_mesh;

    struct link_runtime {
      frame_reader reader;
      bool stream = false;
      size_t transport_index = 0;

      bool hello_sent = false;
      bool remote_hello_valid = false;

      microseconds opened_at{ 0 };
      microseconds last_rx{ 0 };
      microseconds last_tx{ 0 };

      microseconds ping_sent_at{ 0 };
      uint64_t ping_token = 0;
      bool ping_outstanding = false;
    };

    link_record* mutable_link(natural_t link_id);
    link_runtime* runtime(natural_t link_id);
    natural_t link_for_conn(size_t transport_index, natural_t conn_id) const;

    link_record& adopt(size_t transport_index, natural_t conn_id, node_id local, link_state initial,
                       const link_caps& caps, bool stream, uint32_t max_frame_size, microseconds now);
    void drop(natural_t link_id);

    ostd::vector<link_record> records;
    ostd::map<natural_t, link_runtime> runtimes;
    ostd::map<std::pair<size_t, natural_t>, natural_t> conn_links;
    natural_t next_link_id = 1;
  };

}  // namespace other

#endif  // OTHER_NETWORK_PEER_MESH_NETWORK_HPP
