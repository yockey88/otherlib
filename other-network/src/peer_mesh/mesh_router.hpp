/**
 * \file peer_mesh/mesh_router.hpp
 **/
#ifndef OTHER_NETWORK_PEER_MESH_MESH_ROUTER_HPP
#define OTHER_NETWORK_PEER_MESH_MESH_ROUTER_HPP

#include "core/defines.hpp"
#include "core/interfaces.hpp"

#include "peer_mesh/link.hpp"
#include "peer_mesh/node_id.hpp"

namespace other {

  /// routing seam: vantage-aware because one mesh may host many actors — a relay
  ///  forwards from ITS seat. dynamic protocols live out-of-tree behind this
  class OTHER_CLASS mesh_router {
    OTHER_ENVIRONMENT_INTERFACE("Network", "MeshRouter");

   public:
    virtual ~mesh_router() = default;

    virtual std::string_view name() const = 0;

    /// the link to forward on from `from`'s seat; nullopt = unreachable
    virtual opt<natural_t> next_hop(node_id from, node_id dst) = 0;

    virtual void on_link_up(const link_record& link) {}
    virtual void on_link_down(const link_record& link) {}
  };

  /// default: reach exactly the nodes you hold an UP link to
  class direct_router final : public mesh_router {
   public:
    std::string_view name() const override { return "direct"; }

    opt<natural_t> next_hop(node_id from, node_id dst) override;

    void on_link_up(const link_record& link) override;
    void on_link_down(const link_record& link) override;

   private:
    ostd::map<std::pair<node_id, node_id>, natural_t> adjacency;
  };

  /// explicit (from, dst) -> link table, set by tests/scenarios
  class static_route_router final : public mesh_router {
   public:
    std::string_view name() const override { return "static"; }

    void set_route(node_id from, node_id dst, natural_t link_id);
    void clear_route(node_id from, node_id dst);
    void clear_routes();

    opt<natural_t> next_hop(node_id from, node_id dst) override;

   private:
    ostd::map<std::pair<node_id, node_id>, natural_t> routes;
  };

}  // namespace other

#endif  // OTHER_NETWORK_PEER_MESH_MESH_ROUTER_HPP
