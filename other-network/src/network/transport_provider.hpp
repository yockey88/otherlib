/**
 * \file network/transport_provider.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_TRANSPORT_PROVIDER_HPP
#define OTHER_NETWORK_NETWORK_TRANSPORT_PROVIDER_HPP

#include <chrono>
#include <deque>
#include <map>
#include <string>

#include <asio/asio.hpp>

#include "core/defines.hpp"
#include "core/fnv.hpp"
#include "core/interfaces.hpp"
#include "thread/slot_registry.hpp"
#include "thread/thread_safety.hpp"

#include "network/io.hpp"

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

  /// one connection's rx scoped to one sink; the registrant owns the binding and must
  ///  keep it alive until pump quiescence after unregistering (same reclaim contract
  ///  as the sinks themselves)
  struct conn_sink_binding {
    natural_t conn_id = 0;
    packet_sink* sink = nullptr;
  };

  class OTHER_CLASS transport_provider {
    OTHER_ENVIRONMENT_INTERFACE("Network", "TransportProvider");

   public:
    transport_provider() = default;
    transport_provider(const transport_provider&) = delete;
    transport_provider& operator=(const transport_provider&) = delete;
    virtual ~transport_provider() = default;

    virtual std::string name() const = 0;
    natural_t hash() const;
    /// the one lowercase-fnv pipeline for transport names
    static natural_t hash_name(std::string_view transport_name);

    // lifecycle called from network thread
    void initialize(network_thread* host_thread, io* net_io);
    void tick();
    void begin_shutdown();
    void shutdown();

    /// registration runs on the main thread only; the network thread iterates the
    ///  registry wait-free during rx fan-out. unregistering only tombstones the entry —
    ///  destroying the sink itself must wait for network-thread quiescence
    ///  (network_system's deferred reclaim owns that)
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

    /// tolerant of absence: the owner strips a dying sink from every provider without
    ///  tracking which ones it was subscribed to. returns whether it was registered here
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

    /// conn-scoped rx: the binding's sink hears exactly one connection. same writer
    ///  discipline and reclaim contract as the transport-wide registry. a binding
    ///  registered before the connection produces bytes sees the stream from its first
    ///  byte; one registered shortly after establishment gets the held prefix replayed
    ///  (bounded — see rx_hold)
    inline void register_conn_sink(conn_sink_binding* binding) {
      ASSERT_MAIN_THREAD();
      OTHER_ASSERT(binding != nullptr && binding->sink != nullptr && binding->conn_id != 0, "Invalid conn sink binding.");
      const bool inserted = conn_scoped_listeners.insert(binding);
      OTHER_ASSERT(inserted, "Conn-scoped sink registry full (capacity {})", conn_scoped_listeners.capacity());
    }
    /// tombstone only; the binding object outlives this call until pump quiescence
    inline bool unregister_conn_sink(conn_sink_binding* binding) {
      ASSERT_MAIN_THREAD();
      OTHER_ASSERT(binding != nullptr, "Cannot unregister a null conn sink binding.");
      return conn_scoped_listeners.erase(binding);
    }

    /// transport operations all on network thread
    void start_listen(natural_t conn_id, const binding_point& endpoint);
    void start_connect(natural_t conn_id, const binding_point& endpoint);

    /// takes ownership: the payload moved off the bus reaches the wire with no
    ///  further copy
    virtual void tx_data(natural_t connection_id, ostd::vector<uint8_t>&& data) = 0;
    virtual void close(natural_t connection_id) = 0;
    virtual void connection_removed(natural_t connection_id) {}

    void rx_data(natural_t connection_id, std::span<const uint8_t> data);
    /// inbound established (a listener produced it) / outbound established (a dial
    ///  completed); both begin an rx hold so a promptly-registered conn sink misses
    ///  nothing, then notify the driver thread
    void connection_accepted(natural_t listener_id, natural_t conn_id, const binding_point& remote);
    void connection_established(natural_t conn_id, const binding_point& remote);
    /// every teardown funnels here exactly once per connection: sink fan-out, route
    ///  retirement, CONNECTION_CLOSED notification with the reason
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

   private:
    constexpr static size_t kMaxPacketSinks = 16;
    constexpr static size_t kMaxConnSinks = 64;
    constexpr static size_t kRxHoldMaxBytes = 256 * 1024;
    constexpr static std::chrono::seconds kRxHoldExpiry{ 2 };

    /// bytes received before any conn-scoped sink existed, replayed to the first one.
    ///  chunk granularity preserved (datagram transports: 1 chunk = 1 datagram).
    ///  net-thread-only
    struct rx_hold {
      std::chrono::steady_clock::time_point opened_at;
      std::deque<ostd::vector<uint8_t>> chunks;
      size_t total = 0;
      bool expired = false;
    };

    network_thread* host_thread = nullptr;
    io* net_io = nullptr;

    slot_registry<packet_sink, kMaxPacketSinks> registered_listeners;
    slot_registry<conn_sink_binding, kMaxConnSinks> conn_scoped_listeners;
    std::map<natural_t, rx_hold> rx_holds;

    bool has_conn_sink(natural_t conn_id) const;
    void deliver_conn_scoped(natural_t conn_id, std::span<const uint8_t> data);
    void deliver_conn_scoped_opened(natural_t conn_id);
    void begin_rx_hold(natural_t conn_id);
    void flush_rx_hold(natural_t conn_id);
    void sweep_rx_holds();
  };

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_TRANSPORT_PROVIDER_HPP