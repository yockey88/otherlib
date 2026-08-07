/**
 * \file peer_mesh/mesh_router.cpp
 **/
#include "peer_mesh/mesh_router.hpp"

namespace other {

  opt<natural_t> direct_router::next_hop(node_id from, node_id dst) {
    if (auto itr = adjacency.find({ from, dst }); itr != adjacency.end()) {
      return itr->second;
    }
    return std::nullopt;
  }

  void direct_router::on_link_up(const link_record& link) {
    adjacency[{ link.local, link.remote }] = link.link_id;
  }

  void direct_router::on_link_down(const link_record& link) {
    if (auto itr = adjacency.find({ link.local, link.remote }); itr != adjacency.end() && itr->second == link.link_id) {
      adjacency.erase(itr);
    }
  }

  void static_route_router::set_route(node_id from, node_id dst, natural_t link_id) {
    routes[{ from, dst }] = link_id;
  }

  void static_route_router::clear_route(node_id from, node_id dst) {
    routes.erase({ from, dst });
  }

  void static_route_router::clear_routes() {
    routes.clear();
  }

  opt<natural_t> static_route_router::next_hop(node_id from, node_id dst) {
    if (auto itr = routes.find({ from, dst }); itr != routes.end()) {
      return itr->second;
    }
    return std::nullopt;
  }

}  // namespace other
