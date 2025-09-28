/**
 * \file server-dev/server.hpp
 **/
#ifndef OTHER_SERVER_HPP
#define OTHER_SERVER_HPP

#include <asio/asio.hpp>
#include <nlohmann/json.hpp>

#include "driver/driver.hpp"

#include "asio/asio/signal_set.hpp"

namespace json = nlohmann;

namespace other {

  class OTHER_CLASS server : public driver {
   public:
    server(const config_table& config)
        : driver(config) {}
    virtual ~server() = default;

    void on_initialize() override;
    void run() override;
    void on_shutdown() override;

   private:
    asio::io_context io_context;
    asio::signal_set signals{ io_context, SIGINT, SIGTERM };

    json::json project_cache;

    bool running = false;

    void update();
    void on_event(SDL_Event* event) override;
  };

}  // namespace other

OTHER_DRIVER(other::server)

#endif  // OTHER_SERVER_HPP