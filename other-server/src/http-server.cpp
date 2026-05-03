/**
 * \file http-server.cpp
 **/
#include "http-server.hpp"

#include "driver/systems/network_system.hpp"

#include "server.hpp"

namespace other {

  http_server::http_server(server* srv_ptr, uint16_t port)
      : port(port), server_ptr(srv_ptr) {
    OTHER_ASSERT(server_ptr != nullptr, "Server pointer cannot be null when initializing HTTP server.");
    CORE_LOG_INFO("HTTP server initialized on port {}.", port);

    binding_point endpoint{ network_system::network_context::kLocalhostAddress, port };
    main_listening_id = server_ptr->listen_at_endpoint(endpoint);
  }

  void http_server::process_data_from_connection(natural_t connection_id, const std::vector<uint8_t>& data) {
    auto itr = std::ranges::find(connections, connection_id);
    if (itr == connections.end()) {
      CORE_LOG_WARN("Received data from connection ID {} that is not in the list of active connections.", connection_id);
    }

    std::stringstream ss;
    ss << "Received data from connection ID " << connection_id << ":\n";
    for (const auto& byte : data) {
      ss << std::hex << static_cast<int>(byte) << " ";
    }
    CORE_LOG_INFO("{}", ss.str());
  }

  void http_server::process_new_connection(natural_t connection_id) {
    connections.push_back(connection_id);
  }

  void http_server::process_closed_connection(natural_t connection_id) {
    auto itr = std::find(connections.begin(), connections.end(), connection_id);
    if (itr != connections.end()) {
      connections.erase(itr);
    } else {
      CORE_LOG_WARN("Received notification of closed connection ID {} that is not in the list of active connections.", connection_id);
    }
  }

}  // namespace other