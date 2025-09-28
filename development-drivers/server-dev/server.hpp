/**
 * \file server-dev/server.hpp
 **/
#ifndef OTHER_SERVER_HPP
#define OTHER_SERVER_HPP

#include <asio/asio.hpp>
#include <nlohmann/json.hpp>

#include "renderer/renderer.hpp"

#include "scene/scene.hpp"

#include "driver/driver.hpp"

#include "asio/asio/signal_set.hpp"
#include "network_thread.hpp"

namespace json = nlohmann;

namespace other {

  constexpr static binding_point main_binding_point{ 0x7F000001, 0xC046 };  // 127.0.0.1:49222

  class OTHER_CLASS server : public driver {
   public:
    server(const config_table& config)
        : driver(config) {}
    virtual ~server() = default;

    void on_initialize() override;
    void run() override;
    void on_shutdown() override;

   private:
    struct network_context {
      asio::io_context io_context;
      asio::signal_set signals;

      network_context()
          : signals(io_context, SIGINT, SIGTERM) {}
    };

    json::json project_cache;

    scene active_scene;
    scope<renderer> renderer;

    signals::bus message_bus;
    scope<network_thread> net_thread = nullptr;

    /// \todo figure out why asio does not like the arena allocator here
    std::unique_ptr<network_context> net_context = nullptr;

    bool running = false;

    void update();
    void on_event(SDL_Event* event) override;
  };

}  // namespace other

OTHER_DRIVER(other::server)

#endif  // OTHER_SERVER_HPP