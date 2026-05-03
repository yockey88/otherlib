/**
 * \file server.cpp
 **/
#include "server.hpp"

#include "core/defines.hpp"
#include "thread/message.hpp"

#include "driver/driver.hpp"

OTHER_DRIVER(other::server)

namespace other {

  void server::on_initialize(const command_line& cmd) {
    config_http_port = configuration().get_value("server.main-http-port", uint16_t(8080));
    http_server_instance = make_scope<http_server>(this, config_http_port);
    OTHER_ASSERT(http_server_instance != nullptr, "Failed to create HTTP server instance.");
    CORE_LOG_INFO("Binding to HTTP port {}.", config_http_port);
  }

  void server::on_shutdown() {
    CORE_LOG_INFO("Server shutdown complete.");
    http_server_instance = nullptr;
  }

  natural_t server::listen_at_endpoint(const binding_point& endpoint) {
    return core_system<network_system>().listen_at_endpoint(endpoint);
  }

  void server::on_data_received(natural_t id, std::vector<uint8_t> data) {
    OTHER_ASSERT(http_server_instance != nullptr, "HTTP server instance is not initialized.");
    http_server_instance->process_data_from_connection(id, data);
  }

  void server::on_new_connection_accepted(natural_t from_connection_id, natural_t connection_id) {
    OTHER_ASSERT(http_server_instance != nullptr, "HTTP server instance is not initialized.");
    http_server_instance->process_new_connection(connection_id);
  }

  void server::on_connection_closed(natural_t connection_id) {
    OTHER_ASSERT(http_server_instance != nullptr, "HTTP server instance is not initialized.");
    http_server_instance->process_closed_connection(connection_id);
  }

}  // namespace other