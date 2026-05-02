/**
 * \file server.hpp
 **/
#ifndef OTHER_SERVER_SERVER_HPP
#define OTHER_SERVER_SERVER_HPP

#include "driver/driver.hpp"

#include "http-server.hpp"

namespace other {

  class OTHER_CLASS server : public driver {
   public:
    server(const command_line& cmd, const config_table& config)
        : driver(cmd, config) {}
    virtual ~server() = default;

    void on_initialize(const command_line& cmd) override;
    void on_shutdown() override;

    natural_t listen_at_endpoint(const binding_point& endpoint);

   private:
    uint16_t config_http_port = 0;

    scope<http_server> http_server_instance;

    void on_new_connection_accepted(natural_t connection_id) override;
  };

}  // namespace other

#endif  // OTHER_SERVER_SERVER_HPP