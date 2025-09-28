/**
 * \file server-dev/server.cpp
 **/
#include "server.hpp"

#include "core/defines.hpp"

#include "rendering-pipelines/empty_pipeline.hpp"

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

    net_context = std::make_unique<network_context>();
    net_context->signals.async_wait([this](std::error_code ec, int signum) {
      if (!ec) {
        CORE_LOG_INFO("Received signal {}, shutting down server...", signum);
        running = false;
        net_context->io_context.stop();
      } else {
        CORE_LOG_ERROR("Error while waiting for signal: {}", ec.message());
      }
    });

    /// launch threads
    ///  - networking thread
    net_thread = make_scope<network_thread>(net_context->io_context, message_bus, main_binding_point);
    net_thread->launch();

    ///  - UI thread
    if (rendering_enabled()) {
      renderer = get_renderer();
      renderer->add_pipeline<empty_pipeline>("UI Pipeline");
    }
  }

  void server::run() {
    while (running) {
      pump_events();

      net_context->io_context.poll();
      if (net_context->io_context.stopped() && running) {
        net_context->io_context.restart();
      }

      update();

      render_data scene_render_data = active_scene.prepare_render_data();
      renderer->begin_frame(&scene_render_data);
      renderer->render();

      renderer->begin_ui_frame();
      if (ImGui::Begin("Window")) {
        ImGui::Text("Server is running...");
      }
      ImGui::End();
      renderer->end_ui_frame();

      renderer->end_frame();
    }
  }

  void server::on_shutdown() {
    CORE_LOG_INFO("Shutting down server...");
    net_context->signals.cancel();

    if (rendering_enabled()) {
      renderer->remove_pipeline("UI Pipeline");
      renderer = nullptr;
    }

    net_thread->shutdown();
    net_thread = nullptr;
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