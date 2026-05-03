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
    CORE_LOG_INFO("[HTTP Initializing] @ port {}.", config_http_port);
    binding_point endpoint{ network_system::network_context::kLocalhostAddress, config_http_port };
    core_system<network_system>().listen_at_endpoint(endpoint);
  }

  void server::on_shutdown() {
    CORE_LOG_INFO("Server shutdown complete.");
  }

  void server::on_data_received(natural_t id, std::span<const uint8_t> data) {
    CORE_LOG_INFO("Data received on connection {}: {} bytes", id, data.size());
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < data.size(); ++i) {
      ss << std::setw(2) << static_cast<int>(data[i]);
      if (i < data.size() - 1) {
        ss << " ";
      }
    }
    std::string data_str(data.begin(), data.end());
    CORE_LOG_INFO("Data (hex): {}", ss.str());
    CORE_LOG_INFO("Data (string): {}", data_str);
  }

  void server::on_new_connection_accepted(natural_t from_connection_id, natural_t connection_id) {
    CORE_LOG_INFO("New connection accepted. From connection ID: {}, New connection ID: {}", from_connection_id, connection_id);
  }

  void server::on_connection_closed(natural_t connection_id) {
    CORE_LOG_INFO("Connection {} closed.", connection_id);
  }

}  // namespace other