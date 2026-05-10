/**
 * \file network/transport_provider.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_TRANSPORT_PROVIDER_HPP
#define OTHER_NETWORK_NETWORK_TRANSPORT_PROVIDER_HPP

#include <string>

#include <asio/asio.hpp>

#include "core/defines.hpp"
#include "thread/message.hpp"

#include "network/io.hpp"

namespace other {

  class network_thread;

  class transport_provider {
   public:
    transport_provider() = default;
    transport_provider(const transport_provider&) = delete;
    transport_provider& operator=(const transport_provider&) = delete;
    virtual ~transport_provider() = default;

    virtual std::string name() const = 0;

    void initialize(network_thread* host_thread, io* net_io);
    void tick();
    void begin_shutdown();
    void shutdown();

    virtual natural_t start_listen(const binding_point& endpoint) = 0;
    virtual natural_t start_connect(const binding_point& endpoint) = 0;

    virtual void tx_data(natural_t connection_id, std::span<const uint8_t> data) = 0;
    virtual void close(natural_t connection_id) = 0;

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

   private:
    network_thread* host_thread = nullptr;
    io* net_io = nullptr;
  };

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_TRANSPORT_PROVIDER_HPP