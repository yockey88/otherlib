/**
 * \file network/tcp/tcp_transport_provider.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_TCP_TCP_TRANSPORT_PROVIDER_HPP
#define OTHER_NETWORK_NETWORK_TCP_TCP_TRANSPORT_PROVIDER_HPP

#include "network/tcp/connection.hpp"
#include "network/tcp/connection_state_machine.hpp"
#include "network/transport_provider.hpp"

namespace other {

  class network_thread;

  class tcp_transport_provider : public socket_transport_provider {
   public:
    virtual ~tcp_transport_provider() = default;

    bool is_stream() const override { return true; }
    link_caps conn_caps(natural_t) const override {
      return { .reliable = true, .ordered = true, .max_frame_size = 0 };
    }

    std::string name() const override { return "TCP"; }

   private:
    friend class connection;

    std::map<natural_t, scope<connection>> active_connections;
    std::map<natural_t, scope<asio::ip::tcp::acceptor>> active_tcp_listeners;
    std::map<natural_t, connection_state_machine> connection_state_machines;
    /// retired connections whose async handlers have not yet drained; destroyed once idle
    std::deque<natural_t> pending_destroy;

    void on_initialize() override;
    void on_tick() override;
    void on_begin_shutdown() override;
    void on_shutdown() override;

    void on_start_listen(natural_t conn_id, const binding_point& endpoint) override;
    void on_start_connect(natural_t conn_id, const binding_point& endpoint) override;
    void tx_data(natural_t connection_id, ostd::vector<uint8_t>&& data) override;
    void net_close(natural_t connection_id) override;
    void connection_removed(natural_t connection_id) override;

    /// connection callbacks, network thread
    void notify_conn_rx(natural_t connection_id, std::span<const uint8_t> data);
    void notify_conn_closed(natural_t connection_id, connection_close_reason reason);
    void notify_connect_failed(natural_t connection_id);
    void notify_connect_succeeded(natural_t connection_id);

    void start_accept_on(natural_t listener_id, const binding_point& endpoint);
    void on_accepted(natural_t listener_id, const binding_point& endpoint, asio::error_code ec, asio::ip::tcp::socket&& socket);

    void handle_state_event(natural_t connection_id, connection_event event);
    void destroy_if_idle(natural_t connection_id);
  };

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_TCP_TCP_TRANSPORT_PROVIDER_HPP