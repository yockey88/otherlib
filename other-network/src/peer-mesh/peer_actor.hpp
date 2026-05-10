/**
 * \file peer-mesh/peer_actor.hpp
 **/
#ifndef OTHER_NETWORK_PEER_MESH_PEER_ACTOR_HPP
#define OTHER_NETWORK_PEER_MESH_PEER_ACTOR_HPP

#include "core/defines.hpp"

namespace other {

  class peer_actor_host;

  class peer_actor {
   public:
    struct peer_metadata {
      natural_t connection_id = 0;
      uint32_t role = 0;
      std::string_view transport_name;
    };
    peer_actor(natural_t peer_id, peer_actor_host* host)
        : peer_id(peer_id), host(host) {}
    virtual ~peer_actor() = default;

    peer_actor(const peer_actor&) = delete;
    peer_actor& operator=(const peer_actor&) = delete;

    virtual void on_connection_established() {}
    virtual void on_connection_lost(std::error_code reason) {}
    virtual void on_packet_received(std::span<const uint8_t> packet) = 0;

    inline natural_t get_peer_id() const { return peer_id; }

   protected:
    void tx_data(std::span<const uint8_t> data);

    peer_metadata metadata() const;
    void disconnect(std::error_code reason);

   private:
    natural_t peer_id;
    peer_actor_host* host;
  };

}  // namespace other

#endif  // OTHER_NETWORK_PEER_MESH_PEER_ACTOR_HPP