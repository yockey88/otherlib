/**
 * \file peer_mesh/provider_link_transport.hpp
 **/
#ifndef OTHER_NETWORK_PEER_MESH_PROVIDER_LINK_TRANSPORT_HPP
#define OTHER_NETWORK_PEER_MESH_PROVIDER_LINK_TRANSPORT_HPP

#include <deque>
#include <map>
#include <mutex>
#include <set>
#include <vector>

#include "core/defines.hpp"
#include "core/scope.hpp"

#include "network/network_thread.hpp"
#include "network/transport_provider.hpp"
#include "peer_mesh/link_transport.hpp"
#include "peer_mesh/packet_sink.hpp"

namespace other {

  /// adapts a net-thread-homed transport_provider onto the byte-mover seam a mesh
  ///  consumes. main-thread object: dial/listen/tx/close ride the bus as commands; rx
  ///  bytes and lifecycle come back through a conn-scoped packet sink into an ordered
  ///  queue drained by pump(). the bus owner must feed CONNECTION_OPENED/CLOSED
  ///  notifications through handle_bus_message (accept attribution and dial failures
  ///  arrive only that way — adapters on a shared provider self-filter by ownership)
  class provider_link_transport final : public link_transport {
   public:
    provider_link_transport(network_thread& thread, transport_provider& provider, const link_caps& caps, bool stream);
    ~provider_link_transport() override;

    std::string_view name() const override { return lowered_name; }
    bool is_stream() const override { return stream; }
    link_caps conn_caps(natural_t conn_id) const override { return default_caps; }

    void bind(callbacks cbs) override { consumer = std::move(cbs); }

    natural_t dial(const net_address& remote) override;
    natural_t listen(const net_address& bind_addr) override;
    void tx(natural_t conn_id, std::span<const uint8_t> bytes) override;
    void close(natural_t conn_id) override;

    /// main thread, once per tick: reclaims retired bindings and drains queued rx
    ///  events into the bound callbacks
    void pump();
    /// main thread: true when the message was one of ours (a connection notification)
    bool handle_bus_message(const message& msg);
    void on_connection_opened(const notification_connection_opened& note);
    void on_connection_closed(const notification_connection_closed& note);

    size_t open_connections() const { return states.size(); }

   private:
    enum class conn_state : uint8_t { DIALING, OPEN, DEAD };

    struct rx_event {
      enum class kind : uint8_t { OPENED, BYTES, CLOSED };
      kind k = kind::BYTES;
      natural_t conn_id = 0;
      std::vector<uint8_t> bytes;
    };

    /// net-thread producer: overrides the delivery entry points so events and bytes
    ///  land in one ordered queue with no job hop and no cross-channel races
    class queue_sink final : public packet_sink {
     public:
      explicit queue_sink(provider_link_transport& owner)
          : packet_sink(nullptr, "provider-link-transport"), owner(owner) {}

      void rx_data(natural_t conn_id, std::span<const uint8_t> data) override {
        owner.push_event({ rx_event::kind::BYTES, conn_id, std::vector<uint8_t>(data.begin(), data.end()) });
      }
      void connection_opened(natural_t conn_id) override {
        owner.push_event({ rx_event::kind::OPENED, conn_id, {} });
      }
      void connection_closed(natural_t conn_id) override {
        owner.push_event({ rx_event::kind::CLOSED, conn_id, {} });
      }

     protected:
      void on_rx_data(natural_t conn_id, std::span<const uint8_t> data) override {}
      void on_connection_opened(natural_t conn_id) override {}
      void on_connection_closed(natural_t conn_id) override {}

     private:
      provider_link_transport& owner;
    };

    network_thread& thread;
    transport_provider& provider;
    link_caps default_caps;
    bool stream = false;
    std::string lowered_name;

    callbacks consumer;
    queue_sink sink;

    std::mutex queue_mutex;
    std::deque<rx_event> queue;

    std::map<natural_t, conn_state> states;
    std::set<natural_t> my_listeners;
    std::map<natural_t, scope<conn_sink_binding>> live_bindings;
    /// tombstoned bindings waiting out the pump epoch before destruction
    std::vector<std::pair<uint64_t, scope<conn_sink_binding>>> binding_graveyard;

    void push_event(rx_event&& ev);
    void send_command(uint16_t id, ostd::vector<uint8_t>&& data);
    void adopt(natural_t conn_id, conn_state initial);
    void retire(natural_t conn_id);
    void sweep_graveyard(bool force);
  };

}  // namespace other

#endif  // OTHER_NETWORK_PEER_MESH_PROVIDER_LINK_TRANSPORT_HPP