/**
 * \file network/tcp/tcp_transport_provider.hpp
 **/
#ifndef OTHERLIB_NETWORK_TCP_TCP_TRANSPORT_PROVIDER_HPP
#define OTHERLIB_NETWORK_TCP_TCP_TRANSPORT_PROVIDER_HPP

#include "network/tcp/connection.hpp"
#include "network/tcp/connection_state_maching.hpp"
#include "network/transport_provider.hpp"

namespace other {

  class network_thread;

  class tcp_transport_provider : public transport_provider {
   public:
    virtual ~tcp_transport_provider() = default;

    std::string name() const override { return "TCP"; }

   private:
    static inline std::atomic<natural_t> connection_id_counter = 1;
    std::map<natural_t, scope<connection>> active_connections;
    std::map<natural_t, scope<asio::ip::tcp::acceptor>> active_tcp_listeners;
    std::map<natural_t, connection_state_machine> connection_state_machines;
    std::deque<natural_t> recently_closed_connections;

    void on_initialize() override;
    void on_tick() override;
    void on_begin_shutdown() override;
    void on_shutdown() override;

    void on_start_listen(natural_t conn_id, const binding_point& endpoint) override;
    void on_start_connect(natural_t conn_id, const binding_point& endpoint) override;
    void tx_data(natural_t connection_id, std::span<const uint8_t> data) override;
    void close(natural_t connection_id) override;
    void connection_removed(natural_t connection_id) override;

    void on_rx_data(natural_t connection_id, std::span<const uint8_t> data) override;
    void on_connection_socket_closed(natural_t connection_id) override;
    void on_connection_socket_broken(natural_t connection_id) override;

    void start_accept_on(natural_t listener_id);
    void on_accepted(natural_t listener_id, const binding_point& endpoint, asio::error_code ec, asio::ip::tcp::socket&& socket);
    void register_new_connection(asio::ip::tcp::socket&& socket, const binding_point& endpoint, natural_t listener_conn_id);
  };

}  // namespace other

#endif  // OTHERLIB_NETWORK_TCP_TCP_TRANSPORT_PROVIDER_HPP