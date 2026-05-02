/**
 * \file server.cpp
 **/
#include "server.hpp"

#include <filesystem>

#include "core/coroutine.hpp"
#include "core/defines.hpp"
#include "thread/message.hpp"

#include "script/scripting_environment.hpp"

#include "driver/driver.hpp"

#include "server_tasks.hpp"

OTHER_DRIVER(other::server)

namespace other {

  void server::on_initialize(const command_line& cmd) {
    config_http_port = configuration().get_value("server.main-http-port", uint16_t(8080));
    CORE_LOG_INFO("Server will attempt to bind to port {} for main HTTP server.", config_http_port);
  }

  void server::on_shutdown() {
    CORE_LOG_INFO("Server shutdown complete.");
  }

}  // namespace other