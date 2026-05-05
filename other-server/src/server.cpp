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

    lua_server = invoke_driver_method<sol::table>("GetHttpServerLuaInterface");
    add_native_lua_function("__server_native_SendHttpResponse", [this](natural_t id, const http::response& response) {
      CORE_LOG_INFO("Sending HTTP response with status code {} to connection {}", response.status_code, id);
      core_system<network_system>().tx_data(id, response.serialize(http::kHttpVersion1_1));
    });
  }

  void server::on_shutdown() {
  }

  void server::on_http_request_received(natural_t id, const http::request& req) {
    // http::response response{ 200 };

    // sol::table req_table = core_system<scripting_system>().get_lua_host().get_lua_state().create_table();
    // req_table["method"] = req.method.name;
    // req_table["path"] = req.path;

    // std::string resp = lua_server["handle_request"](req_table);
    // response.set_body(resp, "text/html");
    // core_system<network_system>().tx_data(id, response.serialize(http::kHttpVersion1_1));
  }

}  // namespace other