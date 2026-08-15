/**
 * \file network/transport_provider.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_TRANSPORT_PROVIDER_HPP
#define OTHER_NETWORK_NETWORK_TRANSPORT_PROVIDER_HPP

#include <deque>
#include <functional>
#include <map>
#include <mutex>
#include <string>

#include <asio/asio.hpp>

#include "core/defines.hpp"
#include "core/fnv.hpp"
#include "core/interfaces.hpp"
#include "core/scope.hpp"
#include "thread/slot_registry.hpp"
#include "thread/thread_safety.hpp"

#include "network/io.hpp"
#include "network/link.hpp"
#include "network/link_sink.hpp"
#include "network/net_address.hpp"

#include "message/messages.hpp"

namespace other {

  class network_thread;
  class packet_sink;

  /// why a connection went away; rides CONNECTION_CLOSED notifications as u16
  enum class connection_close_reason : uint16_t {
    NONE = 0,
    LOCAL_CLOSE = 1,
    REMOTE_CLOSED = 2,
    READ_ERROR = 3,
    WRITE_ERROR = 4,
    BACKPRESSURE = 5,
    CONNECT_FAILED = 6,
    SHUTDOWN = 7,
  };

  enum class transport_home : uint8_t {
    MAIN_THREAD = 0,
    NET_THREAD = 1,
  };

  /// one connection's bytes scoped to one observer; the registrant owns the struct and
  ///  must keep it alive until reclaim quiescence after unregistering (sink contract)
  struct conn_observer {
    natural_t conn_id = 0;
    packet_sink* sink = nullptr;
  };

  /// how bytes move and how links are established. a provider creates one link_sink per
  ///  seam-established connection — the sink IS the link; a mesh adopts it. raw consumers
  ///  (observers, bus rows) ride the same provider without sinks
  class OTHER_CLASS transport_provider {
    OTHER_ENVIRONMENT_INTERFACE("Network", "TransportProvider");

   public:
    /// main-thread accept handoff: the accepted connection's sink + the listener it arrived on
    using accept_delegate = std::function<void(link_sink&, natural_t listener_id)>;

    transport_provider() = default;
    transport_provider(const transport_provider&) = delete;
    transport_provider& operator=(const transport_provider&) = delete;
    virtual ~transport_provider();

    virtual std::string name() const = 0;
    natural_t hash() const;
    /// the one lowercase-fnv pipeline for transport names
    static natural_t hash_name(std::string_view transport_name);

    virtual transport_home execution_home() const = 0;
    virtual bool is_stream() const = 0;
    virtual link_caps conn_caps(natural_t conn_id) const = 0;
    /// platform-authenticated remote node id; 0 = the transport attests nothing
    virtual node_id attested_remote(natural_t conn_id) const { return 0; }

    /// establishment — main thread. a successful dial has already created the
    ///  connection's sink (sink_of is live on return); accepts create theirs at accept
    virtual natural_t dial(const net_address& remote) = 0;
    virtual natural_t listen(const net_address& bind_addr, accept_delegate on_accept) = 0;
    virtual void tx(natural_t conn_id, std::span<const uint8_t> bytes) = 0;
    virtual void close(natural_t conn_id) = 0;

    /// main thread: the listener stops minting sinks; the socket keeps listening (raw)
    void release_listener(natural_t listener_id);

    link_sink* sink_of(natural_t conn_id);
    /// main thread: the mesh is done with the sink; destruction defers past any
    ///  concurrent provider-home read (swept by maintain())
    void release_sink(natural_t conn_id);
    /// main thread, once per mesh tick: dispatch queued accepts + reclaim released sinks
    void maintain();

    /// boundary observers — raw bytes on the provider's home thread. main-thread
    ///  registration; unregister only tombstones (reclaim contract as before)
    inline void register_packet_sink(packet_sink* sink) {
      ASSERT_MAIN_THREAD();
      OTHER_ASSERT(sink != nullptr, "Cannot register a null packet sink.");
      OTHER_ASSERT(!registered_listeners.contains(sink), "Packet sink is already registered.");

      CORE_LOG_TRACE("[TRANSPORT {}] Registering packet sink at address {:p}", name(), static_cast<const void*>(sink));
      const bool inserted = registered_listeners.insert(sink);
      OTHER_ASSERT(inserted, "Packet sink registry full (capacity {})", registered_listeners.capacity());
      on_registered_packet_sink(sink);
    }
    virtual void on_registered_packet_sink(packet_sink* sink) {}

    inline bool unregister_packet_sink(packet_sink* sink) {
      ASSERT_MAIN_THREAD();
      OTHER_ASSERT(sink != nullptr, "Cannot unregister a null packet sink.");

      if (!registered_listeners.erase(sink)) {
        return false;
      }
      CORE_LOG_TRACE("[TRANSPORT {}] Unregistering packet sink at address {:p}", name(), static_cast<const void*>(sink));
      on_unregistered_packet_sink(sink);
      return true;
    }
    virtual void on_unregistered_packet_sink(packet_sink* sink) {}

    inline void register_conn_observer(conn_observer* observer) {
      ASSERT_MAIN_THREAD();
      OTHER_ASSERT(observer != nullptr && observer->sink != nullptr && observer->conn_id != 0, "Invalid conn observer.");
      const bool inserted = conn_scoped_listeners.insert(observer);
      OTHER_ASSERT(inserted, "Conn observer registry full (capacity {})", conn_scoped_listeners.capacity());
    }
    inline bool unregister_conn_observer(conn_observer* observer) {
      ASSERT_MAIN_THREAD();
      OTHER_ASSERT(observer != nullptr, "Cannot unregister a null conn observer.");
      return conn_scoped_listeners.erase(observer);
    }

   protected:
    constexpr static size_t kMaxPacketSinks = 16;
    constexpr static size_t kMaxConnObservers = 64;

    /// provider-home helpers for implementations
    link_sink& create_sink(natural_t conn_id);
    void destroy_all_sinks();
    bool has_delegate(natural_t listener_id);
    void store_delegate(natural_t listener_id, accept_delegate delegate);
    /// net-home: queue for maintain(); main-home: dispatch straight through
    void queue_accept(natural_t listener_id, natural_t conn_id);
    void dispatch_accept(natural_t listener_id, natural_t conn_id);

    void observers_rx(natural_t conn_id, std::span<const uint8_t> data);
    void observers_opened(natural_t conn_id);
    void observers_closed(natural_t conn_id);

    /// single-writer discipline: only ever from the registering thread's context
    void reset_observer_registries();

    /// reclaim gate: main-home destroys on the next maintain; net-home overrides with
    ///  the pump-epoch discipline
    virtual uint64_t reclaim_epoch_now() { return ++reclaim_counter; }
    virtual bool reclaim_safe(uint64_t marked) { return true; }

   private:
    slot_registry<packet_sink, kMaxPacketSinks> registered_listeners;
    slot_registry<conn_observer, kMaxConnObservers> conn_scoped_listeners;

    std::mutex sink_mutex;
    ostd::map<natural_t, scope<link_sink>> sinks;
    ostd::vector<std::pair<uint64_t, scope<link_sink>>> sink_graveyard;

    std::mutex accept_mutex;
    ostd::map<natural_t, accept_delegate> accept_delegates;
    std::deque<std::pair<natural_t, natural_t>> pending_accepts;  // {listener, conn}

    uint64_t reclaim_counter = 0;

    void sweep_graveyard(bool force);
  };

  /// the net-thread engine room: asio providers (tcp/udp) live on the network thread and
  ///  take establishment/tx/close as bus commands posted by the seam surface
  class OTHER_CLASS socket_transport_provider : public transport_provider {
   public:
    transport_home execution_home() const final { return transport_home::NET_THREAD; }

    // seam surface — main thread, rides the bus
    natural_t dial(const net_address& remote) override;
    natural_t listen(const net_address& bind_addr, accept_delegate on_accept) override;
    void tx(natural_t conn_id, std::span<const uint8_t> bytes) override;
    void close(natural_t conn_id) override;

    /// main thread, at registration — the bus/id-generator the seam surface posts through
    void attach_main(network_thread* main_bus);

    // lifecycle called from network thread
    void initialize(network_thread* host_thread, io* net_io);
    void tick();
    void begin_shutdown();
    void shutdown();

    void start_listen(natural_t conn_id, const binding_point& endpoint);
    void start_connect(natural_t conn_id, const binding_point& endpoint);

    /// takes ownership: the payload moved off the bus reaches the wire with no
    ///  further copy
    virtual void tx_data(natural_t connection_id, ostd::vector<uint8_t>&& data) = 0;
    virtual void net_close(natural_t connection_id) = 0;
    virtual void connection_removed(natural_t connection_id) {}

    /// net-thread rx/lifecycle entry points: observers fan out, the connection's sink
    ///  (when one exists) hears its own bytes, the driver gets notified
    void rx_data(natural_t connection_id, std::span<const uint8_t> data);
    void connection_accepted(natural_t listener_id, natural_t conn_id, const binding_point& remote);
    void connection_established(natural_t conn_id, const binding_point& remote);
    void connection_socket_closed(natural_t connection_id, connection_close_reason reason);

    virtual bool is_reliable() const { return true; }
    virtual bool is_ordered() const { return true; }
    virtual bool is_datagram() const { return false; }

   protected:
    inline network_thread& host_thread_ref() {
      OTHER_ASSERT(host_thread != nullptr, "Transport provider is not initialized with a host thread.");
      return *host_thread;
    }
    inline io& net_io_ref() {
      OTHER_ASSERT(net_io != nullptr, "Transport provider is not initialized with a network io context.");
      return *net_io;
    }

    virtual void on_initialize() = 0;
    virtual void on_tick() = 0;
    virtual void on_begin_shutdown() {}
    virtual void on_shutdown() = 0;

    virtual void on_start_listen(natural_t conn_id, const binding_point& endpoint) = 0;
    virtual void on_start_connect(natural_t conn_id, const binding_point& endpoint) = 0;

    virtual void on_rx_data(natural_t connection_id, std::span<const uint8_t> data) {}
    virtual void on_connection_accepted(natural_t listener_id, natural_t conn_id, const binding_point& remote) {}
    virtual void on_connection_established(natural_t conn_id, const binding_point& remote) {}
    virtual void on_connection_socket_closed(natural_t connection_id, connection_close_reason reason) {}

    uint64_t reclaim_epoch_now() override;
    bool reclaim_safe(uint64_t marked) override;

   private:
    network_thread* host_thread = nullptr;
    network_thread* main_bus = nullptr;
    io* net_io = nullptr;

    void send_command(uint16_t id, ostd::vector<uint8_t>&& data);
  };

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_TRANSPORT_PROVIDER_HPP
