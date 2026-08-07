/**
 * \file peer_mesh/peer_graph.hpp
 **/
#ifndef OTHER_NETWORK_PEER_MESH_PEER_GRAPH_HPP
#define OTHER_NETWORK_PEER_MESH_PEER_GRAPH_HPP

#include "core/defines.hpp"
#include "data-structures/graph.hpp"

#include "peer_mesh/peer_record.hpp"


namespace other {

  class peer_graph {
   public:
    using graph_t = graph<peer_record>;

    const graph_t& get_graph() const { return graph; }
    graph_t& get_graph() { return graph; }

   private:
    graph_t graph;
  };

}  // namespace other

#endif  // OTHER_NETWORK_PEER_MESH_PEER_GRAPH_HPP