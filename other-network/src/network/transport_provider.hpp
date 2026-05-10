/**
 * \file network/transport_provider.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_TRANSPORT_PROVIDER_HPP
#define OTHER_NETWORK_NETWORK_TRANSPORT_PROVIDER_HPP

#include <string>

#include <asio/asio.hpp>

#include "core/defines.hpp"
#include "core/fnv.hpp"
#include "thread/thread_safety.hpp"

#include "network/io.hpp"

#include "message/messages.hpp"

namespace other {

  class network_thread;
  class packet_sink;

  class transport_provider {
   public:
    transport_provider() = default;
    transport_provider(const transport_provider&) = delete;
    transport_provider& operator=(const transport_provider&) = delete;
    virtual ~transport_provider() = default;

    virtual std::string name() const = 0;
    inline natural_t hash() const {
      return FNV(
        name() |
        std::views::transform([](unsigned char c) { return std::tolower(c); }) |
        std::ranges::to<std::string>()
      );
    }

    // lifecycle called from network thread
    void initialize(network_thread* host_thread, io* net_io);
    void tick();
    void begin_shutdown();
    void shutdown();

    // called from main thread
    inline void register_packet_sink(packet_sink* sink) {
      ASSERT_MAIN_THREAD();
      OTHER_ASSERT(sink != nullptr, "Cannot register a null packet sink.");
      OTHER_ASSERT(!registered_listeners.contains(sink), "Packet sink is already registered.");
      registered_listeners.insert(sink);
      on_registered_packet_sink(sink);
    }
    virtual void on_registered_packet_sink(packet_sink* sink) {}

    inline void unregister_packet_sink(packet_sink* sink) {
      ASSERT_MAIN_THREAD();
      OTHER_ASSERT(sink != nullptr, "Cannot unregister a null packet sink.");
      OTHER_ASSERT(registered_listeners.contains(sink), "Packet sink is not registered and cannot be unregistered.");
      registered_listeners.erase(sink);
      on_unregistered_packet_sink(sink);
    }
    virtual void on_unregistered_packet_sink(packet_sink* sink) {}

    /// transport operations all on network thread
    void start_listen(natural_t conn_id, const binding_point& endpoint);
    void start_connect(natural_t conn_id, const binding_point& endpoint);

    virtual void tx_data(natural_t connection_id, std::span<const uint8_t> data) = 0;
    virtual void close(natural_t connection_id) = 0;
    virtual void connection_removed(natural_t connection_id) {}

    void rx_data(natural_t connection_id, std::span<const uint8_t> data);
    void connection_socket_closed(natural_t connection_id);
    void connection_socket_broken(natural_t connection_id);

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
    virtual void on_connection_socket_closed(natural_t connection_id) {}
    virtual void on_connection_socket_broken(natural_t connection_id) {}

   private:
    network_thread* host_thread = nullptr;
    io* net_io = nullptr;

    std::set<packet_sink*> registered_listeners;
  };

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_TRANSPORT_PROVIDER_HPP