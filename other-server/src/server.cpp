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
    binding_point endpoint{ network_system::network_context::kLocalhostAddress, config_http_port };

    core_system<network_system>().listen_at_endpoint(endpoint);
    invoke_driver_method("InitializeHttpServer", config_http_port);
  }

  void server::on_shutdown() {
  }

  void server::on_http_request_received(natural_t id, const http::request& req) {
    http::response response{ 200 };
    response.set_body("<div>Hello, World!</div>", "text/html");
    core_system<network_system>().tx_data(id, response.serialize(http::kHttpVersion1_1));
  }

}  // namespace other