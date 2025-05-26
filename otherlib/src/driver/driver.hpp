/**
 * \file driver/driver.hpp
 **/
#ifndef OTHER_DRIVER_DRIVER_HPP
#define OTHER_DRIVER_DRIVER_HPP

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
    void run();
    void shutdown();

    static std::pair<driver*, std::string> create(const config_table& config);
    static void destroy(const std::string& name, driver* instance);

   protected:
    const config_table& configuration() const {
      return config;
    }

    virtual void on_initialize() = 0;
    virtual void on_run() = 0;
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
  #define DRIVER_NEW(name, config) new name(*config)
#endif
#ifndef DRIVER_DELETE
  #define DRIVER_DELETE(instance) delete instance
#endif

#define OTHER_DRIVER(name)                                                                                       \
  OTHER_PLUGIN(pbrt_sandbox)                                                                                     \
  OTHER_API other::driver* create_driver(const other::config_table* config) { return DRIVER_NEW(name, config); } \
  OTHER_API void destroy_driver(other::driver* instance) { DRIVER_DELETE(instance); }

}  // namespace other

#endif  // OTHER_DRIVER_DRIVER_HPP