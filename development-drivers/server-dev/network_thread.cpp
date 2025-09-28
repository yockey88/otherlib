/**
 * \file server-dev/network_thread.cpp
 **/
#include "network_thread.hpp"

namespace other {

  void network_thread::report_connection_closed(natural_t client_id) {
    auto itr = client_endpoints.find(client_id);
    if (itr != client_endpoints.end()) {
      CORE_LOG_DEBUG("Connection closed for client {}", client_id);
      client_endpoints.erase(itr);
      --current_connections;
    } else {
      CORE_LOG_WARN("Attempted to remove non-existent client {}", client_id);
    }
  }

  void network_thread::report_connection_error(client* cli, const asio::error_code& ec) {
    OTHER_ASSERT(cli != nullptr, "Client pointer is null");
    CORE_LOG_ERROR("Connection error for client {}: {}", cli->client_id, ec.message());
    report_connection_closed(cli->client_id);
  }

  void network_thread::on_initialize() {
    CORE_LOG_DEBUG("Network thread binding to [{}]", binding);
    net_context->acceptor = asio::ip::tcp::acceptor(net_context->io_context, asio::ip::tcp::endpoint(asio::ip::address_v4(binding.ip), binding.port));
    net_context->acceptor.async_accept([this](asio::error_code ec, asio::ip::tcp::socket&& socket) {
      accept_connections(std::move(socket), ec);
    });
  }

  void network_thread::on_start() {
  }

  void network_thread::on_shutdown() {
    CORE_LOG_DEBUG("Shutting down network thread...");
    for (auto& [id, conn] : client_endpoints) {
      conn.connection.shutdown();
    }

    while (client_endpoints.size() > 0) {
      std::this_thread::sleep_for(std::chrono::microseconds(50));
    }

    client_endpoints.clear();
    net_context->acceptor.close();
    net_context = nullptr;
  }

  void network_thread::pump_thread() {
  }

  void network_thread::accept_connections(asio::ip::tcp::socket&& socket, const asio::error_code& ec) {
    if (ec && ec == asio::error::operation_aborted) {
      return;
    } else if (!ec && current_connections >= max_connections) {
      CORE_LOG_WARN("Maximum connections reached, rejecting new connection from {}", socket.remote_endpoint().address().to_string());
      socket.close();
      return;
    }

    if (!ec) {
      natural_t session_id = get_next_connection_id();
      auto [itr, success] = client_endpoints.emplace(session_id, connections{
                                                                   .session_id = session_id,
                                                                   .endpoint = binding_point{ socket.remote_endpoint().address().to_v4().to_uint(), static_cast<uint16_t>(socket.remote_endpoint().port()) },
                                                                   .connection = client(this, session_id, net_context->io_context, std::move(socket)),
                                                                 });
      if (!success) {
        CORE_LOG_ERROR("Failed to add new connection to client endpoints");
        return;
      }

      CORE_LOG_INFO("Accepted new connection with session ID {}", itr->second.session_id);

      ++current_connections;
      itr->second.connection.start_read();

      // Continue accepting new connections
      net_context->acceptor.async_accept([this](asio::error_code ec, asio::ip::tcp::socket socket) {
        accept_connections(std::move(socket), ec);
      });
    } else {
      CORE_LOG_ERROR("Error accepting connection: {}", ec.message());
    }
  }

}  // namespace other