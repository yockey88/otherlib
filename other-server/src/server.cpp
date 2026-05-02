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

namespace other {

  void server::on_initialize(const command_line& cmd) {
  }

  void server::on_shutdown() {
    CORE_LOG_INFO("Server shutdown complete.");
  }

}  // namespace other