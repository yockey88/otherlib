/**
 * \file peer_mesh/peer_actor_host.hpp
 **/
#ifndef OTHER_NETWORK_PEER_MESH_PEER_ACTOR_HOST_HPP
#define OTHER_NETWORK_PEER_MESH_PEER_ACTOR_HOST_HPP

#include <concepts>
#include <vector>

#include "core/defines.hpp"
#include "core/job_system.hpp"
#include "core/ref_counted.hpp"

#include "message/message.hpp"

namespace other {

  template <typename T>
  concept peer_actor_host =
    std::derived_from<T, ref_counted> &&
    requires(T t, /* install_context& ctx, */ natural_t tick, natural_t local_host_id, natural_t conn_id, std::span<const uint8_t> data, std::error_code reason) {
      { T::kPahId } -> std::convertible_to<natural_t>;
      { t.name() } -> std::convertible_to<std::string_view>;
      // { t.install(ctx) } -> std::same_as<void>;
      { t.tick_network(tick) } -> std::same_as<void>;
      { t.uninstall() } -> std::same_as<void>;
      { t.drain_for_frame_bus(tick) } -> std::same_as<void>;
      { t.dispatch_to_local_host(local_host_id, conn_id, data) } -> std::same_as<void>;
      { t.dispatch_open_to_local_host(local_host_id, conn_id) } -> std::same_as<void>;
      { t.dispatch_close_to_local_host(local_host_id, conn_id, reason) } -> std::same_as<void>;
    };

  template <natural_t Id>
  class basic_peer_host : public ref_counted {
   public:
    static constexpr natural_t kPahId = Id;

    // virtual void install(install_context& ctx) = 0;
    virtual void tick_network(natural_t tick) {}
    virtual void uninstall() {}

    virtual void dispatch_to_local_host(natural_t local_host_id, natural_t conn_id, std::span<const uint8_t> data) {}
    virtual void dispatch_open_to_local_host(natural_t local_host_id, natural_t conn_id) {}
    virtual void dispatch_close_to_local_host(natural_t local_host_id, natural_t conn_id, std::error_code reason) {}

    virtual void drain_for_frame_bus(natural_t tick) {}
  };

}  // namespace other

#endif  // OTHER_NETWORK_PEER_MESH_PEER_ACTOR_HOST_HPP