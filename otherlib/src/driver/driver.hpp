/**
 * \file driver/driver.hpp
 **/
#ifndef OTHERLIB_DRIVER_DRIVER_HPP
#define OTHERLIB_DRIVER_DRIVER_HPP

#include <queue>

#include <asio/asio.hpp>
#include <asio/asio/signal_set.hpp>
#include <nlohmann/json.hpp>

#include "core/command_line.hpp"
#include "core/config_table.hpp"
#include "core/coroutine.hpp"
#include "core/defines.hpp"
#include "event/event_system.hpp"
#include "thread/message.hpp"
#include "thread/message_bus.hpp"
#include "thread/messages.hpp"

#include "dotnet/dotnet_assembly.hpp"
#include "network/network_thread.hpp"
#include "renderer/renderer.hpp"

#include "scene/scene_graph.hpp "

#include "plugin/plugin.hpp"
#include "vm/other_device.hpp"

namespace json = nlohmann;

namespace other {

  class driver_thread;

  struct environment_event;

  class OTHER_CLASS driver {
   public:
    driver(const config_table& config)
        : config(config) {}
    virtual ~driver() = default;

    void initialize(const command_line& cmd);
    virtual void run() = 0;
    void shutdown();

    static std::pair<driver*, std::string> create(const config_table& config);
    static void destroy(const std::string& name, driver* instance);

    void write_id_at_address(uint16_t address, natural_t id);
    void emit_instruction(const instruction& op);
    void driver_step_device();

    const config_table& configuration() const {
      return config;
    }

    scope<event_system>& get_event_system() {
      OTHER_ASSERT(net_context != nullptr, "Network context is not initialized in driver.");
      OTHER_ASSERT(net_context->events != nullptr, "Event system is not initialized in driver.");
      return net_context->events;
    }
    void set_scene_to_active(natural_t scene_id);

   protected:
    struct network_context {
      asio::io_context io_context;
      asio::signal_set signals;

      scope<event_system> events = nullptr;
      natural_t netw_thread_heartbeat_timeout_id = 0;

      message_bus net_thread_message_bus;
      scope<network_thread> net_thread = nullptr;

      constexpr static binding_point main_binding_point{ 0x7f000001, 49222 };

      network_context()
          : signals(io_context, SIGINT, SIGTERM) {}
    };
    /// \todo figure out why asio does not like the arena allocator here
    std::unique_ptr<network_context> net_context = nullptr;

    struct pending_ack {
      using on_ack = std::function<void(message_header, const std::vector<uint8_t>&)>;
      using on_timeout = std::function<void(message_header)>;

      natural_t id = 0;

      message_header header;
      microseconds timeout_duration = microseconds(0);
      std::chrono::time_point<std::chrono::steady_clock> sent_time;

      on_ack ack_callback = nullptr;
      on_timeout timeout_callback = nullptr;

      asio::steady_timer timer;

      constexpr auto operator<=>(const pending_ack& other) const {
        return sent_time.time_since_epoch() <=> other.sent_time.time_since_epoch();
      }
    };
    natural_t next_pending_ack_id = 1;
    std::deque<pending_ack> pending_acks;

    struct pending_response {
      using on_response = std::function<void(message_header, const std::vector<uint8_t>&)>;
      using on_timeout = std::function<void(message_header)>;

      natural_t id = 0;

      message_header header;
      std::chrono::time_point<std::chrono::steady_clock> sent_time;

      on_response response_callback = nullptr;
      on_timeout timeout_callback = nullptr;

      asio::steady_timer timer;

      constexpr auto operator<=>(const pending_response& other) const {
        return header <=> other.header;
      }
    };
    natural_t next_pending_response_id = 1;
    std::deque<pending_response> pending_responses;

    struct timeout {
      using on_timeout = std::function<void(natural_t)>;

      natural_t id = 0;
      asio::steady_timer timer;
    };
    natural_t next_timeout_id = 1;
    std::deque<timeout> pending_timeouts;

    virtual void on_initialize(const command_line& cmd) = 0;
    virtual void on_shutdown() = 0;

    virtual void catch_signal(int signal) {}

    bool rendering_enabled() const {
      auto* renderer_backend_subsystem = subsystem<renderer_backend>::get();
      return renderer_backend_subsystem != nullptr && renderer_backend_subsystem->has_backend();
    }

    bool should_shutdown() const {
      return shutdown_requested;
    }

    natural_t create_new_scene(const std::string_view name);
    scene* get_scene(natural_t id);
    scene* get_active_scene();

    filepath get_project_cache();

    void pump_events();

    void handle_session_event_rx_message(message&& msg);

    void handle_request_session_information(integer_t session_id, message&& msg);
    void handle_response_session_information(integer_t session_id, message&& msg);

    virtual std::string get_project_name() const { return ""; }

    virtual void on_event(SDL_Event* event) {}
    virtual void on_event(environment_event* event) {}

    /// notifications
    virtual void handle_notification_session_check_in(message&& msg) {}
    virtual void handle_notification_session_closed(message&& msg) {}
    /// acknowledgments
    virtual void handle_acknowledgement_ack(message&& msg) {}
    /// control messages
    virtual void handle_control_ping(message&& msg) {}
    virtual void handle_control_pong(message&& msg) {}
    /// command messages
    /// request messages
    /// response messages
    virtual void handle_response(message&& msg) {}
    virtual void handle_session_information_response(integer_t session_id, session_information_response&& response) {}
    /// session events
    /// error alerts
    virtual void handle_error_alert(message&& msg) {}

    scope<renderer> get_renderer() const;

    ref<assembly> load_dotnet_module(const std::string_view module_path);
    void unload_dotnet_module(ref<assembly> module_id);

    void launch_detached_process(const filepath& working_dir, const filepath& exe_name, const std::vector<std::string>& args);

    natural_t send_message_and_wait_acknowledgment(message&& msg, microseconds timeout, pending_ack::on_ack ack_callback, pending_ack::on_timeout timeout_callback);
    void cancel_acknowledgment(natural_t ack_id);

    void send_message_and_detach_response(message&& msg, pending_response::on_response response_callback);
    natural_t send_message_and_wait_response(message&& msg, microseconds timeout, pending_response::on_response response_callback, pending_response::on_timeout timeout_callback);
    void cancel_response(natural_t response_id);

    natural_t set_timeout(microseconds duration, timeout::on_timeout timeout_callback);
    void clear_timeout(natural_t timeout_id);

    void process_network_thread_messages(message&& msg);

    void post_coroutine(task coro) {
      add_live_coroutine(std::move(coro));
    }

    template <typename T>
      requires requires(T t) { T{}; }
    decltype(auto) get_config_value(const std::string_view section, const std::string_view key, T default_value = {}) {
      return configuration().get_value(std::format("{}.{}", section, key), default_value);
    }

    other_command_device core_device;

   private:
    friend class driver_interface;

    bool shutdown_requested = false;
    config_table config;

    std::queue<instruction> emitted_instructions;

    std::vector<ref<assembly>> loaded_dotnet_modules;

    struct live_coroutine {
      task handle;
    };
    std::vector<live_coroutine> live_coroutines;

    scene* active_scene = nullptr;
    scope<scene_graph> project_scene_graph = nullptr;

    natural_t add_scene_to_scene_graph(const filepath& scene_path);
    natural_t create_empty_scene(const std::string_view name);
    natural_t get_id_of_scene(const std::string_view name);

    void add_live_coroutine(task handle);
    void poll_coroutines();

    template <typename T>
    T get_value_from_node(const toml::node& node, const T& default_value) const {
      return get_value_from_node<T>(toml::node_view<const toml::node>{ node }, default_value);
    }
  };

#ifndef DRIVER_NEW
  #define DRIVER_NEW(name, config) other::arena_allocator<name>{}.allocate(*config)
#endif
#ifndef DRIVER_DELETE
  #define DRIVER_DELETE(instance) other::arena_allocator<other::driver>{}.free(instance)
#endif

#define OTHER_APPLICATION_DRIVER(name) \
  std::string get_project_name() const override { return name; }

#define OTHER_DRIVER(name)                                                                                       \
  OTHER_PLUGIN(name)                                                                                             \
  OTHER_API other::driver* create_driver(const other::config_table* config) { return DRIVER_NEW(name, config); } \
  OTHER_API void destroy_driver(other::driver* instance) { DRIVER_DELETE(instance); }

#define RUN_DRIVER(name, config)                           \
  {                                                        \
    other::driver* runtime = create_driver(&config);       \
    if (!runtime) {                                        \
      CORE_LOG_ERROR("Failed to create {} driver", #name); \
      return other::exit_code::FAILURE;                    \
    }                                                      \
    runtime->initialize(cmd);                              \
    runtime->run();                                        \
    runtime->shutdown();                                   \
    destroy_driver(runtime);                               \
  }

#ifdef OTHER_APPLICATION
  static inline std::vector<void (*)(SDL_Event*)> event_callbacks;

  static inline void pump_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      subsystem<renderer_backend>::get()->handle_event(&event);
      for (auto& callback : event_callbacks) {
        if (callback) {
          callback(&event);
        }
      }
    }
  }

  static inline void add_event_callback(void (*callback)(SDL_Event*)) {
    if (callback) {
      event_callbacks.push_back(callback);
    } else {
      CORE_LOG_ERROR("Cannot add a null event callback.");
    }
  }
#endif  // OTHER_APPLICATION

}  // namespace other

#endif  // OTHERLIB_DRIVER_DRIVER_HPP