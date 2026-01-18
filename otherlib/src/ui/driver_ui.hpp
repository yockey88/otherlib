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

      /// environment windows
      BUILTIN_WINDOW_TYPE_DATABASE,
      BUILTIN_WINDOW_CONSOLE,

      /// application windows
      BUILTIN_WINDOW_VIEWPORT,
      BUILTIN_WINDOW_SCENE_HIERARCHY,
      BUILTIN_WINDOW_ASSET_BROWSER,
      BUILTIN_WINDOW_PROPERTY_INSPECTOR,

      NUM_BUILTIN_WINDOW_TYPES,
      INVALID_WINDOW_TYPE = NUM_BUILTIN_WINDOW_TYPES
    };

    constexpr static std::array<const std::string_view, NUM_BUILTIN_WINDOW_TYPES> kBuiltinWindowNames{
      "none",

      /// environment windows
      "type-database",
      "console",

      /// application windows
      "viewport",
      "scene-hierarchy",
      "asset-browser",
      "property-inspector",
    };

    driver_ui(driver* drv)
        : driver_ptr(drv) {}
    virtual ~driver_ui() = default;

    void initialize();
    void render();
    void shutdown();

    std::vector<std::string> get_open_window_names() const;

    void open_window(const std::string_view window_name);
    void close_window(const std::string_view window_name);

   private:
    struct builtin_window {
      builtin_window_type type = BUILTIN_WINDOW_NONE;
      bool open = false;
      uint32_t id = 0;
      scope<ui_window> window_ptr = nullptr;

      std::string_view get_name() const {
        return kBuiltinWindowNames[static_cast<size_t>(type)];
      }
    };

    bool main_menu_bar_open = false;
    driver* driver_ptr = nullptr;
    builtin_window builtin_windows[NUM_BUILTIN_WINDOW_TYPES];

    void initialize_builtin_windows();
    void shutdown_builtin_windows();

    void open_builtin_window(builtin_window_type type);
    void close_builtin_window(builtin_window_type type);
  };

}  // namespace other

#endif  // OTHERLIB_UI_DRIVER_UI_HPP