/**
 * \file server.hpp
 **/
#ifndef OTHER_SERVER_SERVER_HPP
#define OTHER_SERVER_SERVER_HPP

#include <chrono>

#include <nlohmann/json.hpp>

#include "core/defines.hpp"

#include "renderer/renderer.hpp"

#include "driver/driver.hpp"

#include "server-ui/server-ui.hpp"

namespace other {

  // 1/10 milli-
  using server_time_unit_conversion = tick_conversion;
  /// 1/10 millisecond duration type
  using server_time_units = tick_duration;

  class OTHER_CLASS server : public driver {
   public:
    server(const command_line& cmd, const config_table& config)
        : driver(cmd, config) {}
    virtual ~server() = default;

    void on_initialize(const command_line& cmd) override;
    void on_shutdown() override;

   private:
    natural_t connection_id = 0;
    uint16_t config_http_port = 0;
  };

}  // namespace other

#endif  // OTHER_SERVER_SERVER_HPP