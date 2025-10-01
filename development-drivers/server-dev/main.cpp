/**
 * \file server-dev/main.cpp
 **/
#include "other.hpp"
#include "server.hpp"

using other::make_scope;
using other::scope;

other::exit_code other_main(const other::command_line& cmd, const other::config_table& config) {
  other::driver* server = create_driver(&config);
  if (!server) {
    CORE_LOG_ERROR("Failed to create simulation driver");
    return other::exit_code::FAILURE;
  }

  server->initialize(cmd);
  server->run();
  server->shutdown();

  destroy_driver(server);
  return other::exit_code::SUCCESS;
}