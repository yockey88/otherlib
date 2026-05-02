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

  void http_server::process_new_connection(natural_t connection_id) {
    connections.push_back(connection_id);
  }

}  // namespace other