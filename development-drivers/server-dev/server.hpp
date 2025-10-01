/**
 * \file server-dev/server.hpp
 **/
#ifndef OTHER_SERVER_HPP
#define OTHER_SERVER_HPP

#include <chrono>

#include <nlohmann/json.hpp>

#include "core/state_machine.hpp"
#include "thread/message_bus.hpp"

#include "network/network_thread.hpp"
#include "renderer/renderer.hpp"

#include "scene/scene.hpp"

#include "driver/driver.hpp"

#include "asio/asio/steady_timer.hpp"

namespace json = nlohmann;

namespace other {

  constexpr static binding_point main_binding_point{ 0x7f000001, 49222 };

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
    server_state_machine state_machine;

    struct event {
      using handler = std::function<void()>;

      natural_t id = 0;
      handler callback = nullptr;
      bool recurring = false;

      server_time_unit duration = server_time_unit::zero();

      asio::steady_timer timer;
    };
    std::deque<event> pending_events;

    struct other_application {
      integer_t id = 0;
      filepath working_directory;
      filepath executable;
      std::vector<std::string> args;
      bool connected = false;
    };
    std::deque<other_application> pending_apps;
    std::map<integer_t, other_application> other_apps;

    struct pending_ack {
      using on_ack = std::function<void(message_header, const std::vector<uint8_t>&)>;
      using on_timeout = std::function<void(message_header)>;

      message_header header;
      std::chrono::microseconds timeout_duration = std::chrono::microseconds(0);
      std::chrono::time_point<std::chrono::steady_clock> sent_time;

      on_ack ack_callback = nullptr;
      on_timeout timeout_callback = nullptr;

      asio::steady_timer timer;

      constexpr auto operator<=>(const pending_ack& other) const {
        return sent_time.time_since_epoch() <=> other.sent_time.time_since_epoch();
      }
    };
    std::deque<pending_ack> pending_acks;
    std::queue<std::chrono::time_point<std::chrono::steady_clock>> ack_removal_queue;

    struct pending_response {
      using on_response = std::function<void(message_header, const std::vector<uint8_t>&)>;

      message_header header;
      std::chrono::time_point<std::chrono::steady_clock> sent_time;

      on_response response_callback = nullptr;

      constexpr auto operator<=>(const pending_response& other) const {
        return header <=> other.header;
      }
    };
    std::deque<pending_response> pending_responses;
    std::queue<std::chrono::time_point<std::chrono::steady_clock>> response_removal_queue;

    void send_message_no_acknowledgment(message&& msg, pending_response::on_response response_callback);
    void send_message_and_wait_acknowledgment(message&& msg, std::chrono::microseconds timeout, pending_ack::on_ack ack_callback, pending_ack::on_timeout timeout_callback);

    natural_t register_event(server_time_unit duration, event::handler callback, bool recurring = false);
    void cancel_event(natural_t event_id);
    void handle_event(natural_t event_id, const asio::error_code& ec);

    void begin_other_application(const filepath& working_dir, const filepath& exe_name, const std::vector<std::string>& args);

    json::json project_cache;

#ifdef OTHER_SERVER_ENABLE_HUB
    scope<renderer> renderer;
#endif

    message_bus net_thread_message_bus;
    scope<network_thread> net_thread = nullptr;

    bool running = false;

    void core_update();

    void on_ack_session_listen_for_network_thread(message_header header, const std::vector<uint8_t>& data);
    void on_timeout_session_listen_for_network_thread(message_header header);
    void update_initializing();

    void on_respond_session_check_in_network_thread(message_header header, const std::vector<uint8_t>& data);
    void update_running();

    void on_shutdown_request();
    void update_shutting_down();

    void on_ack_shutdown_request_network_thread(message_header header, const std::vector<uint8_t>& data);
    void on_timeout_shutdown_request_network_thread(message_header header);
    void update_shut_down();

    void on_event(SDL_Event* event) override;

    void process_network_thread_messages(message&& msg);

    void handle_notification_session_closed(message&& msg);

    void handle_acknowledgement_ack(message&& msg);

    // void handle_control_ping(message&& msg);
    void handle_control_pong(message&& msg);

    void handle_response(message&& msg);

    void renderer_server_ui();
    void validate_object_and_render_project(const json::json& json_obj);
  };

}  // namespace other

OTHER_DRIVER(other::server)

#endif  // OTHER_SERVER_HPP