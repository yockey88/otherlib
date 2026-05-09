/**
 * \file server.hpp
 **/
#ifndef OTHER_SERVER_SERVER_HPP
#define OTHER_SERVER_SERVER_HPP

#include "http/http.hpp"

#include "driver/driver.hpp"

namespace other {

  class OTHER_CLASS server : public driver {
   public:
    server(const command_line& cmd, const config_table& config)
        : driver(cmd, config) {}
    virtual ~server() = default;

    void on_early_initialize(const command_line& cmd) override;
    void on_initialize(const command_line& cmd) override;
    void on_shutdown() override;

   private:
    uint16_t config_http_port = 0;
    filepath mount_directory;

    sol::table lua_server;

    void on_http_request_received(natural_t id, const http::request& req) override;
  };

}  // namespace other

#endif  // OTHER_SERVER_SERVER_HPP