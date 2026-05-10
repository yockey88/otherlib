/**
 * \file network/transport_provider.cpp
 **/
#include "network/transport_provider.hpp"

#include "network/network_thread.hpp"

#include "message/messages.hpp"


namespace other {

  void transport_provider::initialize(network_thread* host_thread, io* net_io) {
    OTHER_ASSERT(host_thread != nullptr, "Host thread pointer is null when initializing transport provider.");
    OTHER_ASSERT(net_io != nullptr, "Network IO pointer is null when initializing transport provider.");

    this->host_thread = host_thread;
    this->net_io = net_io;

    on_initialize();
  }

  void transport_provider::tick() {
    OTHER_ASSERT(net_io != nullptr, "Network IO pointer is null when ticking transport provider.");
    on_tick();
  }

  void transport_provider::begin_shutdown() {
    on_begin_shutdown();
  }

  void transport_provider::shutdown() {
    on_shutdown();

    net_io = nullptr;
    host_thread = nullptr;
  }

  void transport_provider::start_listen(natural_t conn_id, const binding_point& endpoint) {
    CORE_LOG_DEBUG("[TRANSPORT {}] Starting to listen at {}", name(), endpoint);
    on_start_listen(conn_id, endpoint);
  }

  void transport_provider::start_connect(natural_t conn_id, const binding_point& endpoint) {
    CORE_LOG_DEBUG("[TRANSPORT {}] Starting to connect to {}", name(), endpoint);
    on_start_connect(conn_id, endpoint);
  }

  void transport_provider::rx_data(natural_t connection_id, std::span<const uint8_t> data) {
    CORE_LOG_DEBUG("[TRANSPORT {}] Received data on connection {}: {} bytes", name(), connection_id, data.size());
    on_rx_data(connection_id, data);
  }

  /// \todo add status field to close notification

  void transport_provider::connection_socket_closed(natural_t connection_id) {
    CORE_LOG_DEBUG("[TRANSPORT {}] connection closed: ID {}", name(), connection_id);
    on_connection_socket_closed(connection_id);

    message msg(NOTIFICATION, CLOSE_CONNECTION);
    notification_close_connection notification_data{
      .connection_id = connection_id,
      .transport_hash = hash(),
    };
    msg.data = serialize_direct(notification_data);

    host_thread_ref().send_to_driver(std::move(msg));
    host_thread_ref().mark_route_recently_closed(connection_id);
  }

  void transport_provider::connection_socket_broken(natural_t connection_id) {
    CORE_LOG_DEBUG("[TRANSPORT {}] connection broken: ID {}", name(), connection_id);
    on_connection_socket_broken(connection_id);

    message msg(NOTIFICATION, CLOSE_CONNECTION);
    notification_close_connection notification_data{
      .connection_id = connection_id,
      .transport_hash = hash(),
    };
    msg.data = serialize_direct(notification_data);

    host_thread_ref().send_to_driver(std::move(msg));
    host_thread_ref().mark_route_recently_closed(connection_id);
  }

}  // namespace other