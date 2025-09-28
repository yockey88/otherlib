/**
 * \file server-dev/server.cpp
 **/
#include "server.hpp"

#include "core/defines.hpp"

namespace other {

  void server::on_initialize() {
    running = true;
    filepath app_folder = get_project_cache("OtherServer");

    std::ifstream file(app_folder);
    if (file.is_open()) {
      file >> project_cache;
      file.close();
    } else {
      CORE_LOG_WARN("Failed to open project cache file at {}", app_folder.string());
    }

    CORE_LOG_DEBUG("Setting up signal handlers for server shutdown");
    signals.async_wait([this](std::error_code ec, int signum) {
      if (!ec) {
        CORE_LOG_INFO("Received signal {}, shutting down server...", signum);
        running = false;
      } else {
        CORE_LOG_ERROR("Error while waiting for signal: {}", ec.message());
      }
    });

    /// launch threads
    ///  - networking thread
    ///  - simulation thread
  }

  void server::run() {
    while (running) {
      pump_events();

      io_context.run();
      if (running) {
        io_context.restart();
      }
    }
  }

  void server::on_shutdown() {
    signals.cancel();
    io_context.stop();
  }

  void server::update() {
  }

  void server::on_event(SDL_Event* event) {
    OTHER_ASSERT(event != nullptr, "Event is null");
    switch (event->type) {
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED: running = false; break;
      default: break;
    }
  }

}  // namespace other