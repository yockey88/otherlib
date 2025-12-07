/**
 * \file server-dev/server.hpp
 **/
#ifndef OTHER_SERVER_HPP
#define OTHER_SERVER_HPP

#include <chrono>

#include <nlohmann/json.hpp>

#include "core/defines.hpp"
#include "core/state_machine.hpp"
#include "event/event_system.hpp"
#include "thread/message_bus.hpp"

#include "network/network_thread.hpp"
#include "renderer/renderer.hpp"

#include "scene/scene.hpp"

#include "driver/driver.hpp"

#include "server-ui/server-ui.hpp"

namespace other {
  namespace detail {

    class message_handler;

  }  // namespace detail

  enum class server_state : natural_t {
    SERVER_STATE_SHUT_DOWN = 0,
    SERVER_STATE_INITIALIZING,
    SERVER_STATE_RUNNING,
    SERVER_STATE_SHUTTING_DOWN,

    NUM_STATES,
  };
  enum class server_event : natural_t {
    SERVER_EVENT_START = 0,
    SERVER_EVENT_READY,
    SERVER_EVENT_STOP,
    SERVER_EVENT_SHUT_DOWN,

    NUM_EVENTS,
  };

  class server_state_machine : public state_machine<server_state, server_event> {
   public:
    server_state_machine()
        : state_machine<server_state, server_event>(server_state::SERVER_STATE_SHUT_DOWN) {
      add_transition(server_state::SERVER_STATE_SHUT_DOWN, server_event::SERVER_EVENT_START, server_state::SERVER_STATE_INITIALIZING);

      add_transition(server_state::SERVER_STATE_INITIALIZING, server_event::SERVER_EVENT_READY, server_state::SERVER_STATE_RUNNING);
      add_transition(server_state::SERVER_STATE_INITIALIZING, server_event::SERVER_EVENT_STOP, server_state::SERVER_STATE_SHUTTING_DOWN);

      add_transition(server_state::SERVER_STATE_RUNNING, server_event::SERVER_EVENT_STOP, server_state::SERVER_STATE_SHUTTING_DOWN);

      add_transition(server_state::SERVER_STATE_SHUTTING_DOWN, server_event::SERVER_EVENT_STOP, server_state::SERVER_STATE_SHUTTING_DOWN);
      add_transition(server_state::SERVER_STATE_SHUTTING_DOWN, server_event::SERVER_EVENT_SHUT_DOWN, server_state::SERVER_STATE_SHUT_DOWN);
    }
    virtual ~server_state_machine() = default;

    void on_enter_state(server_state new_state) override {
      CORE_LOG_DEBUG("Server state changed to {}", new_state);
    }
  };

  // 1/10 milli-
  using server_time_unit_conversion = std::ratio<1, 10000>;
  /// 1/10 millisecond duration type
  using server_time_unit = std::chrono::duration<uint64_t, server_time_unit_conversion>;

  class OTHER_CLASS server : public driver {
   public:
    server(const config_table& config)
        : driver(config) {}
    virtual ~server() = default;

    void on_initialize(const command_line& cmd) override;
    void run() override;
    void on_shutdown() override;

    void catch_signal(int signum) override;

   private:
    friend class detail::message_handler;

    server_state_machine state_machine;

    struct other_application {
      integer_t id = 0;

      natural_t session_info_request_resp_id = 0;

      opt<filepath> working_directory;

      opt<filepath> executable;
      opt<std::string> name;

      std::vector<std::string> args;
      bool connected = false;

      std::string get_name() const;
    };
    std::deque<other_application> pending_apps;
    std::map<integer_t, other_application> other_apps;

    json::json project_cache;

    scope<renderer> renderer;
    scene active_scene;

    scope<server_ui> ui_ptr = nullptr;

    bool running = false;

    void core_update();

    void update_initializing();
    void update_running();
    void update_shutting_down();
    void update_shut_down();

    void on_event(SDL_Event* event) override;

    void validate_project_and_launch(const json::json& project_entry);
    void begin_other_application(const json::json& project_entry);

    void on_ack_control_ping_network_thread(message_header header, const std::vector<uint8_t>& data);
    void on_timeout_control_ping_network_thread(message_header header);

    void on_ack_session_listen_for_network_thread(message_header header, const std::vector<uint8_t>& data);
    void on_timeout_session_listen_for_network_thread(message_header header);

    void on_ack_shutdown_request_network_thread(message_header header, const std::vector<uint8_t>& data);
    void on_timeout_shutdown_request_network_thread(message_header header);

    void on_respond_session_check_in_network_thread(message_header header, const std::vector<uint8_t>& data);

    void send_session_information_request(integer_t session_id, other_application* app = nullptr);
    void handle_session_information_response(integer_t session_id, session_information_response&& response) override;

    void on_response_request_session_information(message_header header, const std::vector<uint8_t>& data);
    void on_timeout_request_session_information_network_thread(message_header header);
    void print_session_information(other_application* app);

    void register_other_application(integer_t session_id, other_application* app);
    void on_shutdown_request();

    void handle_notification_session_check_in(message&& msg) override;
    void handle_notification_session_closed(message&& msg) override;
    void handle_acknowledgement_ack(message&& msg) override;
    void handle_control_pong(message&& msg) override;
    void handle_response(message&& msg) override;

    task validate_and_build_other_application(const std::string& name, const filepath& folder, const filepath& env_config_path);
  };

}  // namespace other

OTHER_DRIVER(other::server)

#endif  // OTHER_SERVER_HPP