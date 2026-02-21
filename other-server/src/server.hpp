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
    server(const config_table& config)
        : driver(config) {}
    virtual ~server() = default;

    void on_initialize(const command_line& cmd) override;
    void on_initialize_rendering() override;
    void on_initialize_ui(scope<driver_ui>& ui_ptr) override;
    void on_update() override;
    void on_ui_render() override;
    void on_shutdown() override;
    void on_shutdown_rendering() override;

   private:
    json::json project_cache;

    scope<server_ui> ui_ptr = nullptr;

    void core_update();

    void update_initializing() override;
    void update_running() override;
    void update_shutting_down() override;

    void validate_project_and_launch(const json::json& project_entry);
    void begin_other_application(const json::json& project_entry);

    void on_respond_session_check_in_network_thread(message_header header, const std::span<const uint8_t> data);

    void on_notification_session_closed(integer_t session_id) override;

    task validate_and_build_other_application(const std::string& name, const filepath& folder, const filepath& env_config_path);
  };

}  // namespace other

OTHER_DRIVER(other::server)

#endif  // OTHER_SERVER_SERVER_HPP