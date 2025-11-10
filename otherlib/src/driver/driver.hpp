/**
 * \file driver/driver.hpp
 **/
#ifndef OTHERLIB_DRIVER_DRIVER_HPP
#define OTHERLIB_DRIVER_DRIVER_HPP

#include <asio/asio.hpp>
#include <asio/asio/signal_set.hpp>

#include "core/command_line.hpp"
#include "core/config_table.hpp"
#include "core/coroutine.hpp"
#include "core/defines.hpp"

#include "dotnet/dotnet_assembly.hpp"
#include "renderer/renderer.hpp"

#include "plugin/plugin.hpp"

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

   protected:
    struct network_context {
      asio::io_context io_context;
      asio::signal_set signals;

      network_context()
          : signals(io_context, SIGINT, SIGTERM) {}
    };
    /// \todo figure out why asio does not like the arena allocator here
    std::unique_ptr<network_context> net_context = nullptr;

    const config_table& configuration() const {
      return config;
    }

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

    void pump_events();
    virtual void on_event(SDL_Event* event) {}
    virtual void on_event(environment_event* event) {}

    scope<renderer> get_renderer() const;

    ref<assembly> load_dotnet_module(const std::string_view module_path);
    void unload_dotnet_module(ref<assembly> module_id);

    void launch_detached_process(const filepath& working_dir, const filepath& exe_name, const std::vector<std::string>& args);

    void post_coroutine(task coro) {
      add_live_coroutine(std::move(coro));
    }

    template <typename T>
      requires requires(T t) { T{}; }
    decltype(auto) get_config_value(const std::string_view section, const std::string_view key, T default_value = {}) {
      return configuration().get_value(std::format("{}.{}", section, key), default_value);
    }

   private:
    bool shutdown_requested = false;
    config_table config;

    std::vector<ref<assembly>> loaded_dotnet_modules;

    struct live_coroutine {
      task handle;
    };
    std::vector<live_coroutine> live_coroutines;

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