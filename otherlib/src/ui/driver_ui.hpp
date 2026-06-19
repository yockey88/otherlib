/**
 * \file ui/driver_ui.hpp
 **/
#ifndef OTHERLIB_UI_DRIVER_UI_HPP
#define OTHERLIB_UI_DRIVER_UI_HPP

#include <cstdint>
#include <string>

#include "renderer/ui/ui_window.hpp"

#include "ui/menu-bar/menu_bar.hpp"

namespace other {

  class driver;
  class dotnet_object;

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
    std::vector<std::string> get_available_window_names() const;

    void open_window(const std::string_view window_name);
    void close_window(const std::string_view window_name);
    bool is_window_open(const std::string_view window_name) const;

    void register_main_menu_bar_menu(const std::string_view menu_name);
    void register_main_menu_bar_menu(const ui::menu& menu);
    void register_main_menu_bar_sub_menu(const std::string_view menu_name, const ui::menu& sub_menu);
    void register_main_menu_bar_menu_item(const std::string_view menu_name, const ui::menu_item& item);

    natural_t register_window(const std::string_view name, scope<ui_window> window);
    template <typename T, typename... Args>
      requires std::derived_from<T, ui_window>
    natural_t register_window(const std::string_view name, Args&&... args) {
      return register_window(name, make_scope<T>(std::forward<Args>(args)...));
    }

    void unregister_window(natural_t id);
    void unregister_window(const std::string_view name);

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

    struct driver_window {
      std::string name;
      natural_t hash;

      bool open = false;
      uint32_t id = 0;

      scope<ui_window> window_ptr = nullptr;
    };
    struct main_menu_bar_menu {
      natural_t hash;
      ui::menu menu;
    };

    bool main_menu_bar_open = false;
    driver* driver_ptr = nullptr;
    builtin_window builtin_windows[NUM_BUILTIN_WINDOW_TYPES];

    ui::menu_bar main_menu_bar;
    std::vector<main_menu_bar_menu> main_menu_bar_menus;

    natural_t window_registry_id = 0;
    std::unordered_map<natural_t, driver_window> custom_windows;

    event_system& events();

    void initialize_builtin_windows();
    void shutdown_builtin_windows();
    void shutdown_custom_windows();

    void open_builtin_window(builtin_window_type type);
    void close_builtin_window(builtin_window_type type);

    void render_builtin_windows();
    void render_custom_windows();

    void open_custom_window(const std::string_view name);
    void close_custom_window(const std::string_view name);
  };

}  // namespace other

#endif  // OTHERLIB_UI_DRIVER_UI_HPP