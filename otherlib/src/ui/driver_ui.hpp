/**
 * \file ui/driver_ui.hpp
 **/
#ifndef OTHERLIB_UI_DRIVER_UI_HPP
#define OTHERLIB_UI_DRIVER_UI_HPP

#include <cstdint>

#include "renderer/ui/ui_window.hpp"

namespace other {

  class driver;

  class driver_ui {
   public:
    enum builtin_window_type {
      BUILTIN_WINDOW_NONE = 0,
      BUILTIN_WINDOW_CONSOLE,
      BUILTIN_WINDOW_VIEWPORT,
      BUILTIN_WINDOW_TYPE_DATABASE,

      NUM_BUILTIN_WINDOW_TYPES,
      INVALID_WINDOW_TYPE = NUM_BUILTIN_WINDOW_TYPES
    };

    driver_ui(driver* drv)
        : driver_ptr(drv) {}
    virtual ~driver_ui() = default;

    void initialize();
    void render();
    void shutdown();

    void open_builtin_window(builtin_window_type type);
    void close_builtin_window(builtin_window_type type);

   private:
    struct builtin_window {
      builtin_window_type type = BUILTIN_WINDOW_NONE;
      bool open = false;
      uint32_t id = 0;
      scope<ui_window> window_ptr = nullptr;
    };

    bool main_menu_bar_open = false;
    driver* driver_ptr = nullptr;
    builtin_window builtin_windows[NUM_BUILTIN_WINDOW_TYPES];

    void initialize_builtin_windows();
    void shutdown_builtin_windows();
  };

}  // namespace other

#endif  // OTHERLIB_UI_DRIVER_UI_HPP