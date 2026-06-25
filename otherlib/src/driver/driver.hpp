/**
 * \file driver/driver.hpp
 **/
#ifndef OTHERLIB_DRIVER_DRIVER_HPP
#define OTHERLIB_DRIVER_DRIVER_HPP

#include <asio/asio.hpp>
#include <asio/asio/signal_set.hpp>
#include <nlohmann/json.hpp>

// clang-format off
#include "core/defines.hpp"
#include "core/build_config.hpp"
// clang-format on

#include "core/command_line.hpp"
#include "core/config_table.hpp"
#include "core/delta_time.hpp"
#include "core/logger.hpp"
#include "event/event_system.hpp"
#include "file/filesystem.hpp"
#include "input/input_system.hpp"

#include "http/http.hpp"
#include "renderer/renderer.hpp"
#include "script/scripting_environment.hpp"

#include "object/scene_object.hpp"

#include "driver/driver_kernel.hpp"
#include "driver/driver_state_machine.hpp"
#include "driver/driver_system.hpp"
#include "driver/driver_tasks.hpp"
#include "driver/subsystem_registry.hpp"
#include "driver/systems/asset_system.hpp"
#include "driver/systems/event_driver_system.hpp"
#include "driver/systems/job_driver_system.hpp"
#include "driver/systems/rendering_system.hpp"
#include "plugin/plugin.hpp"
#include "plugin/plugin_interface.hpp"
#include "scripting/bindings.hpp"
#include "scripting/dotnet_bindings/driver_bindings.hpp"
#include "scripting/interface_registry.hpp"
#include "scripting/scene_interface.hpp"
#include "ui/driver_ui.hpp"
#include "ui/field_editor_registry.hpp"
#include "vm/other_device.hpp"

#include "message/message.hpp"

namespace json = nlohmann;

namespace other {

  class driver_thread;
  struct asset;

  class OTHER_CLASS driver {
   public:
    constexpr static const std::string_view kDynamicDriverFactorySymbolName = "otherlib_create_driver";
    constexpr static const std::string_view kDynamicDriverDestroySymbolName = "otherlib_destroy_driver";

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

    driver(const command_line& cmd, const config_table& config);
    virtual ~driver();

    const metadata& get_metadata() const { return driver_metadata; }

    void initialize(const command_line& cmd, const subsystem_registry& registry);
    virtual void run();
    void shutdown();

    static std::pair<driver*, std::string> create(const command_line& cmd, const config_table& config);
    static void destroy(const std::string& name, driver* instance);

    natural_t begin_asset_load(const filepath& asset_path);
    void begin_asset_unload(natural_t asset_id);

    natural_t add_model_source_asset(const std::string& name, const std::vector<vertex>& vertices, const std::vector<index>& indices);
    natural_t add_scene_asset(scene* scene_ptr, opt<filepath> scene_path = std::nullopt);
    natural_t add_rendering_pipeline_asset(const std::string_view name, const pipeline_definition& definition);
    natural_t get_asset_hash(natural_t asset_id) const;
    natural_t get_asset_state(natural_t asset_id) const;
    natural_t get_asset_id_from_path(const filepath& path) const;

    void process_driver_event(driver_event event);
    void request_shutdown();

    std::string get_driver_info_string(const std::string_view str) const;

    void confirm_initialization();
    void confirm_assets_clean();
    void confirm_network_thread_shutdown();

    void trigger_event(const std::string& event_name, const value& data = {});

    std::string get_project_name() const;
    std::string get_project_description() const;
    std::string get_project_author() const;
    std::string get_project_version() const;
    bool should_auto_play_scenes() const;

    bool project_loaded() const;

    scene* get_active_scene();
    renderer& get_renderer();

    asset* get_asset(natural_t asset_id);

    inline bool network_enabled() const {
      return !configuration().get_value<bool>("networking.force-disable", false);
    }
    inline bool rendering_enabled() const {
      return !subsystem<renderer_backend>::inert;
    }
    inline bool scripting_enabled() const {
      return !subsystem<scripting_environment>::inert;
    }
    inline bool physics_enabled() const {
      return !subsystem<physics_environment>::inert;
    }

    inline driver_kernel& get_kernel() {
      OTHER_ASSERT(driver_kernel_ptr != nullptr, "Driver kernel is not initialized.");
      return *driver_kernel_ptr;
    }

    inline const config_table& configuration() const {
      return config;
    }

    inline job_system& get_job_system() {
      OTHER_ASSERT(driver_kernel_ptr != nullptr, "Driver kernel is not initialized.");
      return driver_kernel_ptr->get_core_system<job_driver_system>().get_job_system();
    }
    inline scope<event_system>& get_event_system() {
      OTHER_ASSERT(driver_kernel_ptr != nullptr, "Driver kernel is not initialized.");
      return driver_kernel_ptr->get_core_system<event_driver_system>().events();
    }
    inline scope<driver_ui>& get_ui() {
      OTHER_ASSERT(driver_kernel_ptr != nullptr, "Driver kernel is not initialized.");
      return driver_kernel_ptr->get_core_system<rendering_system>().get_driver_ui();
    }

    inline interface_registry& get_interface_registry() {
      return interfaces;
    }

    inline ui::field_editor_registry& get_field_editors() {
      return field_editors;
    }

    inline driver_state current_driver_state() const {
      return state_machine.get_current_state();
    }
    inline mode get_current_mode() const {
      return current_mode;
    }

    inline void queue_project_load(const filepath& project_file) {
      std::lock_guard lock(runtime_state.mutex);
      runtime_state.queued_project_file = project_file;
    }

    template <typename T>
      requires requires(T t) { T{}; }
    decltype(auto) get_config_value(const std::string_view toml_path, T default_value = {}) const {
      return configuration().get_value<T>(toml_path, default_value);
    }

    void input_event(const input_state_change_event& event);
    // void data_received(natural_t id, std::vector<uint8_t> data);
    // void handle_http_request_received(natural_t id, const http::request& req);
    // void new_connection_accepted(natural_t from_connection_id, natural_t connection_id);
    // void connection_closed(natural_t connection_id);

    /// \todo remove this and read input map from the input map asset, or allow it to get
    ///         built from a script callback to lua or .NET scripts
    virtual void on_build_driver_input_map(input_map& map) {}
    virtual void on_viewport_resize(const glm::vec2& size) {}

    // other
    virtual void on_render() {}
    virtual void on_ui_render() {}

   protected:
    template <typename R = void, typename... Args>
    R invoke_driver_method(const std::string_view method_name, Args&&... args) {
      return invoke_driver_script_function<R, Args...>(method_name, std::forward<Args>(args)...);
    }

    template <typename Fn>
    void add_native_lua_function(const std::string_view function_name, Fn&& function) {
      if (!scripting_enabled()) {
        CORE_LOG_WARN("Scripting is not enabled, cannot add native Lua function '{}'", function_name);
        return;
      }

      auto* env = subsystem<scripting_environment>::get();
      OTHER_ASSERT(env != nullptr, "scripting_environment null in add_native_lua_function!");

      sol::state& lua_state = env->get_lua_host().get_lua_state();
      if (auto existing_fn = lua_state[function_name.data()]; existing_fn.valid()) {
        CORE_LOG_ERROR("Cannot add native Lua function '{}': a function with that name already exists in the Lua environment", function_name);
        return;
      }

      lua_state.set_function(function_name.data(), std::forward<Fn>(function));
    }

    // void add_interface(const std::string_view interface_name, dotnet_interface_info info);
    natural_t add_interface(const std::string_view interface_name, sol::table inteface_table);
    // void add_interface(const std::string_view interface_name, plugin* plugin_ptr);

    void http_request_received(natural_t id, const http::request& req);

    virtual void on_early_initialize() {}
    virtual void on_initialize() = 0;
    virtual void on_system_initialization() {}
    virtual void on_initialization_confirm() {}
    virtual void on_update() {}
    virtual void update_initializing() {}
    virtual void update_running() {}
    virtual void update_shutting_down() {}
    virtual void on_shutdown() = 0;
    virtual void on_shutdown_request() {}
    virtual void on_shutdown_confirm() {}

    virtual void on_input_event(const input_state_change_event& event) {}
    /// notifications
    virtual void on_data_received(natural_t id, std::span<const uint8_t> data) {}
    virtual void on_http_request_received(natural_t id, const http::request& req) {}
    virtual void on_new_connection_accepted(natural_t main_connection_id, natural_t connection_id) {}
    virtual void on_connection_closed(natural_t connection_id) {}
    /// acknowledgments
    /// control messages
    /// command messages
    /// request messages
    /// response messages
    /// session events
    /// error alerts

    template <typename T>
      requires std::derived_from<T, driver_system>
    T& core_system() {
      OTHER_ASSERT(driver_kernel_ptr != nullptr, "Driver kernel is not initialized.");
      return driver_kernel_ptr->template get_core_system<T>();
    }

   private:
    friend class driver_interface;
    friend class driver_state_machine;
    friend void bind_otherlib_driver_lua_functions(lua_host& lua_host, driver* host_driver);
    friend void bindings::native_driver_request_shutdown();
    friend native_string bindings::native_driver_get_project_name();

    struct running_state {
      std::mutex mutex;
      opt<filepath> queued_project_file;

      bool shutdown_requested = false;
    };
    struct shutdown_state {
      bool network_thread_shutdown = false;
      bool asset_manager_shutdown = false;
      bool project_unloaded = false;

      inline bool ready_to_shutdown(driver* drv) const {
        return drv->current_driver_state() == driver_state::DRIVER_STATE_SHUTTING_DOWN &&
          network_thread_shutdown && asset_manager_shutdown && project_unloaded;
      }
    };
    running_state runtime_state;
    shutdown_state shutdown_state;

    config_table config;
    command_line cmd_line;

    metadata driver_metadata;
    scope<driver_kernel> driver_kernel_ptr = nullptr;

    delta_time frame_delta_time;

    driver_state_machine state_machine;
    mode current_mode = CORE;

    interface_registry interfaces;
    ui::field_editor_registry field_editors;

    template <typename R>
    R default_return() {
      if constexpr (std::is_same_v<R, void>) {
        return;
      } else {
        return R{};
      }
    }

    template <typename R = void, typename... Args>
    R invoke_driver_script_function(const std::string_view function_name, Args&&... args) {
      if (!scripting_enabled()) {
        CORE_LOG_WARN("Scripting is not enabled, cannot invoke driver script function '{}'", function_name);
        return default_return<R>();
      }

      auto* env = subsystem<scripting_environment>::get();
      OTHER_ASSERT(env != nullptr, "scripting_environment null in invoke_driver_script_function!");

      sol::state& lua_state = env->get_lua_host().get_lua_state();
      sol::object func_obj = lua_state[function_name.data()];
      if (!func_obj.valid() || func_obj.get_type() != sol::type::function) {
        CORE_LOG_WARN("No valid Lua function named '{}' found to invoke", function_name);
        return default_return<R>();
      }

      sol::function func = func_obj.as<sol::function>();
      try {
        sol::object result = func(std::forward<Args>(args)...);
        if constexpr (!std::is_same_v<R, void>) {
          if (result.is<R>()) {
            return result.as<R>();
          } else {
            CORE_LOG_WARN("Lua function '{}' did not return expected type", function_name);
            return default_return<R>();
          }
        } else {
          return default_return<R>();
        }
      } catch (const sol::error& e) {
        CORE_LOG_ERROR("Error invoking Lua function '{}': {}", function_name, e.what());
        return default_return<R>();
      } catch (...) {
        CORE_LOG_ERROR("Unknown error invoking Lua function '{}'", function_name);
        return default_return<R>();
      }
    }

    metadata build_metadata();

    void load_client();
    filepath get_project_cache();

    void update();
    void render();

    void on_project_loaded();
    void on_project_unloaded();

    void launch_detached_process(const filepath& working_dir, const filepath& exe_name, const std::vector<std::string>& args);

    void handle_driver_event_with_lua_table(const std::string_view event_name, const sol::table& event_data);

    void begin_shutdown_sequence();

    template <typename T>
    T get_value_from_node(const toml::node& node, const T& default_value) const {
      return get_value_from_node<T>(toml::node_view<const toml::node>{ node }, default_value);
    }
  };

}  // namespace other

#define OTHER_DRIVER(name)                                                                                                              \
  OTHER_PLUGIN(name_##_otherlib_driver, "", "", "")                                                                                     \
  extern "C" OTHER_API ::other::driver* otherlib_create_driver(const ::other::command_line* cmd, const ::other::config_table* config) { \
    return ::other::arena_allocator<name>{}.allocate(*cmd, *config);                                                                    \
  }                                                                                                                                     \
  extern "C" OTHER_API void otherlib_destroy_driver(::other::driver* instance) {                                                        \
    ::other::arena_allocator<::other::driver>{}.free(instance);                                                                         \
  }

#define OTHER_NO_DRIVER()                                                                                                               \
  extern "C" OTHER_API ::other::driver* otherlib_create_driver(const ::other::command_line* cmd, const ::other::config_table* config) { \
    return nullptr;                                                                                                                     \
  }                                                                                                                                     \
  extern "C" OTHER_API void otherlib_destroy_driver(::other::driver* instance) {                                                        \
  }

extern "C" {
extern ::other::driver* otherlib_create_driver(const ::other::command_line* cmd, const ::other::config_table* config);
extern void otherlib_destroy_driver(::other::driver* instance);
}

#endif  // OTHERLIB_DRIVER_DRIVER_HPP