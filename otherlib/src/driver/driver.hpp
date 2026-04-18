/**
 * \file driver/driver.hpp
 **/
#ifndef OTHERLIB_DRIVER_DRIVER_HPP
#define OTHERLIB_DRIVER_DRIVER_HPP

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

#include "network/message_handler.hpp"
#include "renderer/renderer.hpp"

#include "object/scene_object.hpp"

#include "driver/driver_kernel.hpp"
#include "driver/driver_state_machine.hpp"
#include "driver/subsystem_registry.hpp"
#include "driver/systems/driver_system.hpp"
#include "driver/systems/event_driver_system.hpp"
#include "driver/systems/network_system.hpp"
#include "driver/systems/rendering_system.hpp"
#include "plugin/plugin.hpp"
#include "scripting/dotnet_bindings/driver_bindings.hpp"
#include "ui/driver_ui.hpp"
#include "vm/other_device.hpp"

namespace json = nlohmann;

namespace other {

  class driver_thread;
  struct asset;

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

    void initialize(const command_line& cmd, const subsystem_registry& registry);
    virtual void run();
    void shutdown();

    static std::pair<driver*, std::string> create(const config_table& config);
    static void destroy(const std::string& name, driver* instance);

    natural_t begin_asset_load(const filepath& asset_path, std::function<void(natural_t)> on_loaded = nullptr);
    natural_t add_model_source_asset(const std::string& name, const std::vector<vertex>& vertices, const std::vector<index>& indices);
    natural_t add_scene_asset(scene* scene_ptr, opt<filepath> scene_path = std::nullopt);
    natural_t get_asset_hash(natural_t asset_id) const;

    void process_driver_event(driver_event event);
    void request_shutdown();
    void send_load_command(const std::string_view scene_name, natural_t scene_id, bool is_empty, bool requires_udp_binding);

    std::string get_driver_info_string(const std::string_view str) const;

    void confirm_initialization();
    void confirm_shutdown();

    void trigger_event(const std::string& event_name, const value& data = {});

    std::string get_project_name() const;
    std::string get_project_description() const;
    std::string get_project_author() const;
    std::string get_project_version() const;
    bool should_auto_play_scenes() const;

    scene* get_active_scene();

    inline bool network_enabled() const {
      return !configuration().get_value<bool>("networking.force-disable", false);
    }
    inline bool rendering_enabled() const {
      auto* rendering_backend = subsystem<renderer_backend>::get();
      return !((rendering_backend->has_backend() && configuration().rendering_backend.value() == "headless") || configuration().force_no_window);
    }
    inline bool scripting_enabled() const {
      return !configuration().get_value<bool>("scripting.force-disable-scripting", false);
    }
    inline bool physics_enabled() const {
      return !configuration().get_value<bool>("physics.force-disable-physics", false);
    }

    inline driver_kernel& get_kernel() {
      OTHER_ASSERT(driver_kernel_ptr != nullptr, "Driver kernel is not initialized.");
      return *driver_kernel_ptr;
    }

    inline const config_table& configuration() const {
      return config;
    }

    inline scope<event_system>& get_event_system() {
      OTHER_ASSERT(driver_kernel_ptr != nullptr, "Driver kernel is not initialized.");
      return driver_kernel_ptr->get_core_system<event_driver_system>().events();
    }
    inline scope<driver_ui>& get_ui() {
      OTHER_ASSERT(driver_kernel_ptr != nullptr, "Driver kernel is not initialized.");
      return driver_kernel_ptr->get_core_system<rendering_system>().get_driver_ui();
    }
    renderer& get_renderer();

    inline driver_state current_driver_state() const {
      return state_machine.get_current_state();
    }
    inline mode get_current_mode() const {
      return current_mode;
    }

    template <typename T>
      requires requires(T t) { T{}; }
    decltype(auto) get_config_value(const std::string_view toml_path, T default_value = {}) const {
      return configuration().get_value(toml_path, default_value);
    }

    /// \todo remove this and read input map from the input map asset, or allow it to get
    ///         built from a script callback to lua or .NET scripts
    virtual void on_build_driver_input_map(input_map& map) {}
    virtual void on_input_event(const input_state_change_event& event) {}
    /// notifications
    virtual void on_notification_session_closed(integer_t session_id) {}
    /// acknowledgments
    /// control messages
    /// command messages
    /// request messages
    /// response messages
    /// session events
    /// error alerts
    virtual void handle_error_alert(message&& msg) {}
    virtual void on_push_scene_object(scene_object* object) {}
    virtual void on_pop_scene_object(scene_object* object) {}
    virtual void on_viewport_resize(const glm::vec2& size) {}
    virtual void on_render() {}
    virtual void on_ui_render() {}

   protected:
    virtual void on_initialize(const command_line& cmd) = 0;
    virtual void on_initialization_confirm() {}
    virtual void on_update() {}
    virtual void update_initializing() {}
    virtual void update_running() {}
    virtual void update_shutting_down() {}
    virtual void on_shutdown() = 0;
    virtual void on_shutdown_request() {}
    virtual void on_shutdown_confirm() {}

   private:
    friend class driver_interface;
    friend class driver_state_machine;
    friend void bindings::native_driver_request_shutdown();
    friend native_string bindings::native_driver_get_project_name();

    struct shutdown_state {
      bool network_thread_shutdown = false;
      bool asset_manager_shutdown = false;
    };
    struct live_coroutine {
      task handle;
    };

    shutdown_state shutdown_state;

    config_table config;
    command_line cmd_line;

    metadata driver_metadata;
    scope<driver_kernel> driver_kernel_ptr = nullptr;

    std::vector<live_coroutine> live_coroutines;

    delta_time frame_delta_time;

    driver_state_machine state_machine;
    mode current_mode = CORE;

    json::json project_cache;

    metadata build_metadata();

    void load_client();
    filepath get_project_cache();

    void update();
    void render();

    void launch_detached_process(const filepath& working_dir, const filepath& exe_name, const std::vector<std::string>& args);

    void post_coroutine(task coro);
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

#ifdef OTHER_PLUGIN_LIBRARY
  #define OTHER_PLUGIN(name)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  \
    extern "C" {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              \
    OTHER_API const char* other_plugin_name() { return #name; }                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               \
    OTHER_API void bind_plugin_systems(other::other_plugin_argv* argv) {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      \
      other::subsystem<other::arena>::set(argv->arena);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       \
      other::subsystem<other::logger>::set(argv->logger);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
      other::subsystem<other::file_system>::set(argv->file_system);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           \
      other::subsystem<other::input_system>::set(argv->input_system);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         \
      other::subsystem<other::type_database>::set(argv->type_database);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       \
      other::subsystem<other::physics_environment>::set(argv->physics_environment);                                                                                                                                                                                                                                                                                                                                                                                                                                                                           \
      other::subsystem<other::renderer_backend>::set(argv->renderer);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         \
      other::subsystem<other::scripting_environment>::set(argv->scripting_environment);                                                                                                                                                                                                                                                                                                                                                                                                                                                                       \
      CORE_LOG_DEBUG("Plugin '{}' bound to subsystems: arena={:p}, logger={:p}, file_system={:p}, input_system={:p}, type_database={:p}, physics_environment={:p}, renderer_backend={:p}, scripting_environment={:p}", #name, static_cast<void*>(argv->arena), static_cast<void*>(argv->logger), static_cast<void*>(argv->file_system), static_cast<void*>(argv->input_system), static_cast<void*>(argv->type_database), static_cast<void*>(argv->physics_environment), static_cast<void*>(argv->renderer), static_cast<void*>(argv->scripting_environment)); \
    }                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         \
    OTHER_API other::driver* create_driver(const other::config_table* config) { return nullptr; }                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
    OTHER_API void destroy_driver(other::driver* instance) {}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 \
    OTHER_API other::driver_system* create_plugin(other::driver* driver_ptr) { return other::arena_allocator<name>{}.allocate(driver_ptr); }                                                                                                                                                                                                                                                                                                                                                                                                                  \
    OTHER_API void destroy_plugin(name* instance) { other::arena_allocator<name>{}.free(instance); }                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
    }
#endif

#define OTHER_DRIVER(name)                                                                                       \
  OTHER_PLUGIN(name)                                                                                             \
  extern "C" {                                                                                                   \
  OTHER_API other::driver* create_driver(const other::config_table* config) { return DRIVER_NEW(name, config); } \
  OTHER_API void destroy_driver(other::driver* instance) { DRIVER_DELETE(instance); }                            \
  }

#endif  // OTHERLIB_DRIVER_DRIVER_HPP