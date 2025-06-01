/**
 * \file driver/driver.hpp
 **/
#ifndef OTHER_DRIVER_DRIVER_HPP
#define OTHER_DRIVER_DRIVER_HPP

#include "core/arena_allocator.hpp"
#include "core/config_table.hpp"
#include "core/defines.hpp"
#include "plugin/plugin.hpp"
#include "renderer/renderer.hpp"

namespace other {

  class driver_thread;

  class OTHER_CLASS driver {
   public:
    driver(const config_table& config)
        : config(config) {}
    virtual ~driver() = default;

    void initialize();
    virtual void run() = 0;
    void shutdown();

    static std::pair<driver*, std::string> create(const config_table& config);
    static void destroy(const std::string& name, driver* instance);

   protected:
    const config_table& configuration() const {
      return config;
    }

    virtual void on_initialize() = 0;
    virtual void on_shutdown() = 0;

    bool should_shutdown() const {
      return shutdown_requested;
    }

    void pump_events();
    virtual void on_event(SDL_Event* event) {}

    scope<renderer> get_renderer() const;

   private:
    bool shutdown_requested = false;
    config_table config;
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

#endif  // OTHER_DRIVER_DRIVER_HPP