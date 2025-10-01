/**
 * \file editor-dev/main.cpp
 **/
#include <asio/asio.hpp>

#include "other.hpp"
#include "runtime.hpp"

other::exit_code other_main(const other::command_line& cmd, const other::config_table& config) {
  CORE_LOG_DEBUG("Starting Other Runtime Driver...");
  CORE_LOG_DEBUG("   - SESSION : {}", cmd.session_id.has_value() ? cmd.session_id.value() : 0);
  CORE_LOG_DEBUG("   - PORT    : {}", cmd.port.has_value() ? cmd.port.value() : 49222);

  other::driver* runtime = create_driver(&config);
  if (!runtime) {
    CORE_LOG_ERROR("Failed to create simulation driver");
    return other::exit_code::FAILURE;
  }

  runtime->initialize(cmd);
  runtime->run();
  runtime->shutdown();

  destroy_driver(runtime);
  return other::exit_code::SUCCESS;
}