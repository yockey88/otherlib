/**
 * \file server.hpp
 **/
#ifndef OTHER_SERVER_SERVER_HPP
#define OTHER_SERVER_SERVER_HPP

#include "driver/driver.hpp"

namespace other {

  class OTHER_CLASS server : public driver {
   public:
    server(const command_line& cmd, const config_table& config)
        : driver(cmd, config) {}
    virtual ~server() = default;

    void on_early_initialize() override;
    void on_initialize() override;
    void on_shutdown() override;

   private:
    uint16_t config_port = 0;
    filepath mount_directory;
  };

}  // namespace other

#endif  // OTHER_SERVER_SERVER_HPP