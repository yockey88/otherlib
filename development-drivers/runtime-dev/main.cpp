/**
 * \file editor-dev/main.cpp
 **/
#include <asio/asio.hpp>

#include "other.hpp"
#include "runtime.hpp"

other::exit_code other_main(const other::command_line& cmd, const other::config_table& config) {
  PROFILE_SECTION("simulation--other_main");
#if 0
  other::driver* runtime = create_driver(&config);
  if (!runtime) {
    CORE_LOG_ERROR("Failed to create simulation driver");
    return other::exit_code::FAILURE;
  }

  runtime->initialize();
  runtime->run();
  runtime->shutdown();

  destroy_driver(runtime);
#else
  asio::io_context io_context;

  asio::ip::tcp::socket socket(io_context);
  asio::ip::tcp::endpoint endpoint(asio::ip::address_v4::loopback(), 49222);
  socket.async_connect(endpoint, [&](const asio::error_code& ec) {
    if (!ec) {
      CORE_LOG_INFO("Connected to server at {}", endpoint.address().to_string());
    } else {
      CORE_LOG_ERROR("Failed to connect to server at {}: {}", endpoint.address().to_string(), ec.message());
    }
  });

  io_context.run();
#endif
  return other::exit_code::SUCCESS;
}