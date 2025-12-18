/**
 * \file ui/driver_ui.cpp
 **/
#include "ui/driver_ui.hpp"

#include "driver/driver.hpp"
#include "ui/console.hpp"
#include "ui/type_database.hpp"
#include "ui/viewport.hpp"

namespace other {

  void driver_ui::initialize() {
    initialize_builtin_windows();
    main_menu_bar_open = driver_ptr->configuration().get_value<bool>("ui.enable-environment-menu-bar", false);
  }

  void driver_ui::render() {
    if (main_menu_bar_open) {
      if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
          if (ImGui::MenuItem("New Project")) {}
          if (ImGui::MenuItem("Open Project")) {}
          ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
      }
    }

    for (size_t i = 0; i < NUM_BUILTIN_WINDOW_TYPES; ++i) {
      auto& window = builtin_windows[i];
      if (window.open && window.window_ptr != nullptr) {
        window.window_ptr->render();
      }
    }
  }

  void driver_ui::shutdown() {
    shutdown_builtin_windows();
  }

  void driver_ui::open_builtin_window(builtin_window_type type) {
    OTHER_ASSERT(type > BUILTIN_WINDOW_NONE && type < NUM_BUILTIN_WINDOW_TYPES, "Invalid builtin window type");
    auto& window = builtin_windows[static_cast<size_t>(type)];
    window.window_ptr->toggle_open();
    window.open = true;
  }

  void driver_ui::close_builtin_window(builtin_window_type type) {
    OTHER_ASSERT(type > BUILTIN_WINDOW_NONE && type < NUM_BUILTIN_WINDOW_TYPES, "Invalid builtin window type");
    auto& window = builtin_windows[static_cast<size_t>(type)];
    window.window_ptr->toggle_close();
    window.open = false;
  }

  void driver_ui::initialize_builtin_windows() {
    for (size_t i = 0; i < NUM_BUILTIN_WINDOW_TYPES; ++i) {
      builtin_window_type type = static_cast<builtin_window_type>(i);
      auto& window = builtin_windows[i];
      window.type = type;
      window.open = false;
      window.id = static_cast<uint32_t>(i);
      switch (type) {
        case BUILTIN_WINDOW_CONSOLE: window.window_ptr = make_scope<ui::console_window>(*driver_ptr->get_event_system()); break;
        case BUILTIN_WINDOW_VIEWPORT: window.window_ptr = make_scope<ui::viewport>(*driver_ptr->get_event_system(), driver_ptr->get_renderer_pointer()); break;
        case BUILTIN_WINDOW_TYPE_DATABASE: window.window_ptr = make_scope<ui::type_database>(*driver_ptr->get_event_system()); break;
        default:
          break;
      }
    }
  }

  void driver_ui::shutdown_builtin_windows() {
  }

}  // namespace other