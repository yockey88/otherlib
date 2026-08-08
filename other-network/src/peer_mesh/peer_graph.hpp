/**
 * \file peer_mesh/peer_graph.hpp
 **/
#ifndef OTHER_NETWORK_PEER_MESH_PEER_GRAPH_HPP
#define OTHER_NETWORK_PEER_MESH_PEER_GRAPH_HPP

#include "core/defines.hpp"
#include "data-structures/graph.hpp"

#include "peer_mesh/node_id.hpp"
#include "peer_mesh/peer_record.hpp"

namespace other {

  /// the mesh's live view: vertices = known nodes, edges = links. edges incident to a resident
  ///  node are authoritative; edges between two remote nodes are advisory (imported topology)
  class peer_graph {
   public:
    using graph_t = graph<peer_record>;

    const graph_t& get_graph() const { return node_graph; }
    graph_t& get_graph() { return node_graph; }

    peer_record& ensure_node(node_id node) {
      if (peer_record* existing = record(node); existing != nullptr) {
        return *existing;
      }
      const natural_t graph_id = node_graph.add_node(peer_record{ .node = node });
      graph_ids[node] = graph_id;
      return *node_graph.ptr_to_node_value(graph_id);
    }

    peer_record* record(node_id node) {
      if (auto itr = graph_ids.find(node); itr != graph_ids.end()) {
        return node_graph.ptr_to_node_value(itr->second);
      }
      return nullptr;
    }
    const peer_record* record(node_id node) const {
      if (auto itr = graph_ids.find(node); itr != graph_ids.end()) {
        return const_cast<graph_t&>(node_graph).ptr_to_node_value(itr->second);
      }
      return nullptr;
    }

    void remove_node(node_id node) {
      if (auto itr = graph_ids.find(node); itr != graph_ids.end()) {
        node_graph.remove_node(itr->second);
        graph_ids.erase(itr);
      }
    }

    bool contains(node_id node) const { return graph_ids.contains(node); }
    size_t size() const { return graph_ids.size(); }

    void add_edge(node_id a, node_id b) {
      ensure_node(a);
      ensure_node(b);
      node_graph.add_edge(graph_ids[a], graph_ids[b]);
      node_graph.add_edge(graph_ids[b], graph_ids[a]);
    }

    void remove_edge(node_id a, node_id b) {
      auto ia = graph_ids.find(a);
      auto ib = graph_ids.find(b);
      if (ia == graph_ids.end() || ib == graph_ids.end()) {
        return;
      }
      node_graph.remove_edge(ia->second, ib->second);
      node_graph.remove_edge(ib->second, ia->second);
    }

    ostd::vector<node_id> neighbors(node_id node) const {
      ostd::vector<node_id> out;
      auto itr = graph_ids.find(node);
      if (itr == graph_ids.end()) {
        return out;
      }
      for (const natural_t neighbor_graph_id : node_graph.get_neighbors(itr->second)) {
        if (const peer_record* rec = const_cast<graph_t&>(node_graph).ptr_to_node_value(neighbor_graph_id); rec != nullptr) {
          out.push_back(rec->node);
        }
      }
      return out;
    }

   private:
    graph_t node_graph;
    /// node_id (u64 identity) -> graph-internal vertex id
    ostd::map<node_id, natural_t> graph_ids;
  };

}  // namespace other

#endif  // OTHER_NETWORK_PEER_MESH_PEER_GRAPH_HPP
