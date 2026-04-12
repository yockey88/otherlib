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
#include "core/delta_time.hpp"
#include "event/event_system.hpp"
#include "input/input_system.hpp"
#include "thread/message.hpp"
#include "thread/message_bus.hpp"

#include "dotnet/dotnet_assembly.hpp"
#include "lua/lua_script.hpp"
#include "network/message.hpp"
#include "network/message_handler.hpp"
#include "network/network_thread.hpp"
#include "renderer/renderer.hpp"

#include "scene/scene_graph.hpp "

#include "driver/acknowledgement_list.hpp"
#include "driver/application_list.hpp"
#include "driver/driver_kernel.hpp"
#include "driver/driver_state_machine.hpp"
#include "driver/response_list.hpp"
#include "driver/timer_list.hpp"
#include "plugin/plugin.hpp"
#include "scripting/dotnet_bindings.hpp"
#include "scripting/dotnet_bindings/driver_bindings.hpp"
#include "ui/driver_ui.hpp"
#include "vm/other_device.hpp"

#include "asset/asset_handler.hpp"
#include "systems/driver_system.hpp"

namespace json = nlohmann;

namespace other {

  class driver_thread;

  class OTHER_CLASS driver {
   public:
    /// the various modes of the driver that can be set
    ///  with '/' commands in the console.
    enum mode {
      CORE,
      SCENE,
      SCENE_OBJECT,
      FILE,
    };
    struct metadata {
      std::string name;
      std::string description;
      std::string author;
      std::string version;
    };
    bool dynamic = false;

    driver(const config_table& config)
        : config(config) {}
    virtual ~driver() = default;

    const metadata& get_metadata() const { return driver_metadata; }

    void initialize(const command_line& cmd);
    virtual void run();
    void shutdown();

    static std::pair<driver*, std::string> create(const config_table& config);
    static void destroy(const std::string& name, driver* instance);

    void trigger_event(const std::string_view name, const value& data);

    void write_id_at_address(uint16_t address, natural_t id);
    void emit_instruction(const instruction& op);
    void execute_driver_command(const std::string& command);
    void driver_step_device();

    natural_t begin_asset_load(const filepath& asset_path, std::function<void(natural_t)> on_loaded = nullptr);

    scene* get_active_scene();
    void new_blank_scene(const std::string_view name);

    void process_driver_event(driver_event event);

    inline const config_table& configuration() const {
      return config;
    }
    inline asio::io_context& get_io_context() {
      OTHER_ASSERT(net_context != nullptr, "Network context is not initialized in driver.");
      return net_context->io_context;
    }

    /**
     * \todo return reference to thing itself not reference to unique pointer for all functions below
     **/

    inline scope<event_system>& get_event_system() {
      OTHER_ASSERT(net_context != nullptr, "Network context is not initialized in driver.");
      OTHER_ASSERT(net_context->events != nullptr, "Event system is not initialized in driver.");
      return net_context->events;
    }

    inline scope<renderer>& get_renderer_pointer() {
      OTHER_ASSERT(rendering_enabled(), "Attempting to access renderer while rendering is disabled. Unexpected behavior or invalid configuration, or a bug in a script.");
      OTHER_ASSERT(renderer_ptr != nullptr, "Renderer is not initialized in driver.");
      return renderer_ptr;
    }

    inline scope<asset_handler>& get_asset_manager() {
      OTHER_ASSERT(asset_mgr != nullptr, "Asset manager is not initialized in driver.");
      return asset_mgr;
    }

    inline scope<scene_graph>& get_scene_graph() {
      OTHER_ASSERT(project_scene_graph != nullptr, "Project scene graph is not initialized.");
      return project_scene_graph;
    }

    void set_scene_to_active(natural_t scene_id);
    void synchronize_active_scene(natural_t scene_id);
    void unload_active_scene();

    inline driver_state current_driver_state() const {
      return state_machine.get_current_state();
    }

    inline mode get_current_mode() const {
      return current_mode;
    }

    void request_shutdown();

    void send_load_command(const std::string_view scene_name, natural_t scene_id, bool is_empty, bool requires_udp_binding);

    natural_t add_scene_to_scene_graph(const filepath& scene_path);
    natural_t create_empty_scene(const std::string_view name);
    natural_t get_id_of_scene(const std::string_view name);

    void open_ui_window(const std::string_view type);
    void close_ui_window(const std::string_view type);
    inline scope<driver_ui>& get_ui() {
      OTHER_ASSERT(driver_ui_ptr != nullptr, "Driver UI is not initialized.");
      return driver_ui_ptr;
    }

    void push_scene_object_to_context_stack(scene_object* object);
    scene_object* pop_scene_object_from_context_stack();

    std::string get_driver_info_string(const std::string_view str) const;

   protected:
    struct network_context {
      asio::io_context io_context;
      asio::signal_set signals;

      scope<event_system> events = nullptr;
      natural_t netw_thread_heartbeat_timeout_id = 0;

      message_bus net_thread_message_bus;
      scope<network_thread> net_thread = nullptr;

      constexpr static uint32_t kLocalhostAddress = 0x7f000001;
      constexpr static uint32_t kPrimarySessionBindingPort = 49222;
      constexpr static uint16_t kServerBroadcastPost0 = 50160;

      constexpr static binding_point main_binding_point{ kLocalhostAddress, kPrimarySessionBindingPort };
      uint16_t next_available_server_port = kServerBroadcastPost0;

      network_context() : signals(io_context, SIGINT, SIGTERM) {}
    };
    scope<network_context> net_context = nullptr;

    acknowledgement_list ack_list;
    response_list resp_list;
    timer_list timeout_list;
    application_list app_list;

    metadata driver_metadata;
    scope<driver_kernel> driver_kernel_ptr = nullptr;

    virtual void on_initialize(const command_line& cmd) = 0;

    input_map get_driver_input_map();
    /// \todo remove this and read input map from the input map asset, or allow it to get
    ///         built from a script callback to lua or .NET scripts
    virtual void on_build_driver_input_map(input_map& map) {}

    void initialize_network_context();
    void load_client();
    void start_network();
    void initialize_rendering();
    virtual void on_initialize_rendering();
    void initialize_ui();
    /// \todo remove this function or make it not take a pointer,
    ///           user driver should be able to load UI from a file or through .NET scripts,
    ///           not hardcoded in C++
    virtual void on_initialize_ui(scope<driver_ui>& ui_ptr) {}

    virtual void on_shutdown() = 0;
    void shutdown_rendering();
    virtual void on_shutdown_rendering();
    void shutdown_ui();
    virtual void on_shutdown_ui() {}

    void catch_signal(int signal);

    virtual void on_shutdown_request() {}
    virtual void on_shutdown_confirm() {}

    bool rendering_enabled() const {
      auto* renderer_backend_subsystem = subsystem<renderer_backend>::get();
      return renderer_backend_subsystem != nullptr && renderer_backend_subsystem->has_backend();
    }

    natural_t create_new_scene(const std::string_view name);
    scene* get_scene(natural_t id);

    renderer& get_renderer_instance();

    filepath get_project_cache();

    inline lua_script& get_envrc_script() {
      OTHER_ASSERT(envrc != nullptr, "Driver environment runtime script is not loaded.");
      return *envrc;
    }

    void update();
    void render();
    void render_ui();

    virtual void on_update() {}
    virtual void update_initializing() {
      if (primary_role == NONE) {
        CORE_LOG_DEBUG("No network role, skipping initialization wait.");
        process_driver_event(driver_event::DRIVER_EVENT_READY);
      }
    }
    virtual void update_running() {}
    virtual void update_shutting_down() {}
    virtual void on_render() {}
    virtual void on_ui_render() {}

    void pump_events();

    void handle_request_session_information(integer_t session_id, message&& msg);
    void handle_response_session_information(integer_t session_id, message&& msg);

    void on_acknowledge_command_environment_load_scene(message_header header, std::span<const uint8_t> data);
    void on_timeout_environment_load_scene(message_header header);
    void request_scene_udp_binding(udp_binding_information address);

    std::string get_project_name() const;
    std::string get_project_description() const;
    std::string get_project_author() const;
    std::string get_project_version() const;
    bool should_auto_play_scenes() const;

    void handle_input_event(const input_state_change_event& event);
    virtual void on_input_event(const input_state_change_event& event) {}

    /// notifications
    void handle_notification_stream_receive_udp_datagram(message&& msg);
    void handle_notification_session_check_in(message&& msg);
    void handle_notification_session_closed(message&& msg);
    virtual void on_notification_session_closed(integer_t session_id) {}

    /// acknowledgments
    void handle_acknowledgement_ack(message&& msg);

    void on_ack_shutdown_request_network_thread(message_header header, const std::span<const uint8_t> data);
    void on_timeout_shutdown_request_network_thread(message_header header);

    void on_ack_session_connect_to(message_header header, const std::span<const uint8_t> data);
    void on_timeout_session_connect_to(message_header header);

    void on_ack_session_listen_for_network_thread(message_header header, const std::span<const uint8_t> data);
    void on_timeout_session_listen_for_network_thread(message_header header);

    /// control messages
    void handle_control_ping(message&& msg);
    void handle_control_pong(message&& msg);

    /// command messages
    void handle_command_environment_load_scene(integer_t session_id, message&& msg);

    /// request messages
    void session_check_in_request(integer_t session_id);
    void session_application_information_request(integer_t session_id, application_list::other_application* app = nullptr);

    /// response messages
    void handle_response(message&& msg);

    void on_respond_session_check_in(message_header header, const std::span<const uint8_t> data);

    void handle_session_information_response(integer_t session_id, session_information_response&& response);
    void print_session_information(application_list::other_application* app);

    void on_respond_new_udp_stream_binding(message_header header, std::span<const uint8_t> data);
    void on_timeout_new_udp_stream_binding(message_header header);

    /// session events
    void handle_session_event_rx_message(message&& msg);

    /// error alerts
    virtual void handle_error_alert(message&& msg) {}

    scope<renderer> get_renderer() const;

    ref<assembly> load_dotnet_module(const std::string_view module_path);
    void unload_dotnet_module(ref<assembly> module_id);

    void launch_detached_process(const filepath& working_dir, const filepath& exe_name, const std::vector<std::string>& args);

    void send_message_and_detach_acknowledgement(message&& msg, message_handler handler);
    natural_t send_message_and_wait_acknowledgment(message&& msg, microseconds timeout, message_handler handler);
    void cancel_acknowledgment(natural_t ack_id);

    void send_message_and_detach_response(message&& msg, message_handler handler);
    natural_t send_message_and_wait_response(message&& msg, microseconds timeout, message_handler handler);
    void cancel_response(natural_t response_id);

    natural_t set_timeout(microseconds duration, timer_list::timeout::on_timeout timeout_callback);
    void clear_timeout(natural_t timeout_id);

    void register_other_application(integer_t session_id, application_list::other_application* app);

    void process_network_thread_messages(message&& msg);

    void post_coroutine(task coro) {
      add_live_coroutine(std::move(coro));
    }

    template <typename T>
      requires requires(T t) { T{}; }
    decltype(auto) get_config_value(const std::string_view toml_path, T default_value = {}) const {
      return configuration().get_value(toml_path, default_value);
    }

    void send_to_network_thread(message&& msg);

   private:
    friend void bindings::native_driver_request_shutdown();
    friend native_string bindings::native_driver_get_project_name();

    // std::unordered_map<system_key, driver_system*> custom_systems;
    // std::array<driver_system*, kNumBuiltinDriverSystems> builtin_systems;
    // std::array<driver_system*, kNumSystemSlots> active_systems;

    struct initialization_state {
    };
    struct shutdown_state {
      bool network_thread_shutdown = false;
      bool asset_manager_shutdown = false;
    };

    initialization_state init_state;
    shutdown_state shutdown_state;

    friend class driver_interface;
    friend class driver_state_machine;

    struct loading_asset {
      using handler = std::function<void(natural_t)>;
      natural_t asset_id = 0;
      handler on_loaded = nullptr;
    };

    enum driver_role {
      SERVER,
      CLIENT,

      NONE,
    };
    /// each driver can be both at the same time,
    ///     but this will take precedence in certain operations
    driver_role primary_role = CLIENT;

    config_table config;
    command_line cmd_line;

    std::queue<instruction> emitted_instructions;

    lua_script* envrc = nullptr;
    std::vector<ref<assembly>> loaded_dotnet_modules;

    struct live_coroutine {
      task handle;
    };
    std::vector<live_coroutine> live_coroutines;

    struct open_stream {
      integer_t stream_id = 0;
      // ...
    };
    std::vector<open_stream> active_streams;

    delta_time frame_delta_time;
    constexpr inline static natural_t kObjectContextStackSize = 16;
    size_t context_stack_top = 0;
    scene_object* context_stack[kObjectContextStackSize] = { nullptr };
    scene* active_scene = nullptr;
    scope<scene_graph> project_scene_graph = nullptr;

    lua_script* driver_main_lua_script = nullptr;
    // dotnet_object* dotnet_window_registry = nullptr;

    opt<integer_t> client_session_id;
    driver_state_machine state_machine;
    other_command_device core_device;
    mode current_mode = CORE;

    scope<renderer> renderer_ptr = nullptr;
    scope<driver_ui> driver_ui_ptr = nullptr;
    scope<asset_handler> asset_mgr = nullptr;
    std::deque<loading_asset> loading_asset_ids;

    json::json project_cache;

    metadata build_metadata();
    void configure_filesystem();

    virtual void on_push_scene_object(scene_object* object) {}
    virtual void on_pop_scene_object(scene_object* object) {}

    void handle_viewport_resize_event(const value& data);
    virtual void on_viewport_resize(const glm::vec2& size) {}
    glm::vec2 viewport_size;

    // void handle_scene_load_empty_event(const value& data);
    // void handle_scene_load_event(const value& data);
    // void handle_scene_unload_event(const value& data);
    // void handle_scene_info_event(const value& data);
    // void handle_scene_playback_command_event(const value& data);

    // void handle_open_ui_window_event(const value& data);
    // void handle_close_ui_window_event(const value& data);

    // void handle_list_driver_default_event(const value& data);
    // void handle_list_driver_windows_event(const value& data);
    // void handle_list_driver_files_event(const value& data);
    // void handle_list_driver_scenes_event(const value& data);
    // void handle_list_driver_assets_event(const value& data);

    // void handle_object_driver_create_event(const value& data);
    // void handle_object_driver_destroy_event(const value& data);
    // void handle_object_driver_push_event(const value& data);
    // void handle_object_driver_pop_event(const value& data);
    // void handle_object_driver_info_event(const value& data);

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

}  // namespace other

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

#if defined(OTHER_STATIC_LIBRARY) && !defined(OTHER_TEST_ENVIRONMENT)
extern "C" {
extern other::driver* create_driver(const other::config_table* config);
extern void destroy_driver(other::driver* instance);
}
#endif

#ifdef OTHER_DYNAMIC_DRIVER
  #define OTHER_PLUGIN(name)                                                            \
    extern "C" {                                                                        \
    OTHER_API const char* other_plugin_name() { return #name; }                         \
    OTHER_API void bind_plugin_systems(other::other_plugin_argv* argv) {                \
      other::subsystem<other::arena>::set(argv->arena);                                 \
      other::subsystem<other::logger>::set(argv->logger);                               \
      other::subsystem<other::file_system>::set(argv->file_system);                     \
      other::subsystem<other::input_system>::set(argv->input_system);                   \
      other::subsystem<other::type_database>::set(argv->type_database);                 \
      other::subsystem<other::physics_environment>::set(argv->physics_environment);     \
      other::subsystem<other::renderer_backend>::set(argv->renderer);                   \
      other::subsystem<other::scripting_environment>::set(argv->scripting_environment); \
    }                                                                                   \
    }

#endif

#ifdef OTHER_STATIC_DRIVER
  #define OTHER_PLUGIN(name)                                              \
    extern "C" {                                                          \
    OTHER_API const char* other_plugin_name() { return nullptr; }         \
    OTHER_API void bind_plugin_systems(other::other_plugin_argv* argv) {} \
    }
#endif

#define OTHER_DRIVER(name)                                                                                       \
  OTHER_PLUGIN(name)                                                                                             \
  extern "C" {                                                                                                   \
  OTHER_API other::driver* create_driver(const other::config_table* config) { return DRIVER_NEW(name, config); } \
  OTHER_API void destroy_driver(other::driver* instance) { DRIVER_DELETE(instance); }                            \
  }

#endif  // OTHERLIB_DRIVER_DRIVER_HPP