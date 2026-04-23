/**
 * \file ui/driver_ui.cpp
 **/
#include "ui/driver_ui.hpp"

#include <algorithm>

#include "core/defines.hpp"

#include "driver/driver.hpp"
#include "ui/asset-browser/asset_browser.hpp"
#include "ui/console/console.hpp"
#include "ui/scene-hierarchy/scene_hierarchy.hpp"
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
          if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
            events().trigger_event("scene.load-scene", value("resources/empty_scene.scene"));
          }
          ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
          ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
      }
    }

    render_builtin_windows();
    render_custom_windows();
  }

  void driver_ui::shutdown() {
    shutdown_custom_windows();
    shutdown_builtin_windows();
  }

  std::vector<std::string> driver_ui::get_open_window_names() const {
    std::vector<std::string> open_windows;
    for (size_t i = 0; i < NUM_BUILTIN_WINDOW_TYPES; ++i) {
      if (i == 0 || i == static_cast<size_t>(INVALID_WINDOW_TYPE)) {
        continue;
      }

      const auto& window = builtin_windows[i];
      if (window.open) {
        open_windows.push_back(std::string(window.get_name()));
      }
    }
    return open_windows;
  }

  void driver_ui::open_window(const std::string_view window_name) {
    CORE_LOG_DEBUG("Request to open UI window: {}", window_name);
    natural_t hash = FNV(window_name);

    auto windows = std::span(builtin_windows, NUM_BUILTIN_WINDOW_TYPES);
    auto it = std::ranges::find_if(windows, [hash](const builtin_window& win) {
      return FNV(kBuiltinWindowNames[static_cast<size_t>(win.type)]) == hash;
    });
    if (it == windows.end()) {  // } || it->type == BUILTIN_WINDOW_NONE || it->type == INVALID_WINDOW_TYPE) {
      open_custom_window(window_name);
    } else {
      if (it->window_ptr == nullptr) {
        CORE_LOG_ERROR("UI window appears '{}' to be unimplemented.", window_name);
        return;
      }

      open_builtin_window(it->type);
    }
  }

  void driver_ui::close_window(const std::string_view window_name) {
    CORE_LOG_DEBUG("Request to close UI window: {}", window_name);
    natural_t hash = FNV(window_name);

    auto windows = std::span(builtin_windows, NUM_BUILTIN_WINDOW_TYPES);
    auto it = std::ranges::find_if(windows, [hash](const builtin_window& win) {
      return FNV(kBuiltinWindowNames[static_cast<size_t>(win.type)]) == hash;
    });
    if (it == windows.end()) {  // || it->type == BUILTIN_WINDOW_NONE || it->type == INVALID_WINDOW_TYPE) {
      close_custom_window(window_name);
    } else {
      if (it->window_ptr == nullptr) {
        CORE_LOG_ERROR("UI window appears '{}' to be unimplemented.", window_name);
        return;
      }

      close_builtin_window(it->type);
    }
  }

  bool driver_ui::is_window_open(const std::string_view window_name) const {
    natural_t hash = FNV(window_name);

    auto windows = std::span(builtin_windows, NUM_BUILTIN_WINDOW_TYPES);
    auto it = std::ranges::find_if(windows, [hash](const builtin_window& win) {
      return FNV(kBuiltinWindowNames[static_cast<size_t>(win.type)]) == hash;
    });
    if (it == windows.end()) {
      return false;
    }
    return it->open;
  }

  event_system& driver_ui::events() {
    OTHER_ASSERT(driver_ptr != nullptr, "Driver pointer is null in driver UI.");
    return *driver_ptr->get_event_system();
  }

  void driver_ui::initialize_builtin_windows() {
    CORE_LOG_DEBUG("Initializing builtin UI windows...");
    for (size_t i = 0; i < NUM_BUILTIN_WINDOW_TYPES; ++i) {
      if (i == 0 || i == static_cast<size_t>(INVALID_WINDOW_TYPE)) {
        continue;
      }

      CORE_LOG_DEBUG("  - initializing builtin window: {}", kBuiltinWindowNames[i]);
      builtin_window_type type = static_cast<builtin_window_type>(i);
      auto& window = builtin_windows[i];
      window.type = type;
      window.open = false;
      window.id = static_cast<uint32_t>(i);

      auto& rendering = driver_ptr->get_kernel().get_core_system<rendering_system>();
      auto& renderer = rendering.get_renderer();
      switch (type) {
        case BUILTIN_WINDOW_TYPE_DATABASE: window.window_ptr = make_scope<ui::type_database>(*driver_ptr->get_event_system()); break;
        case BUILTIN_WINDOW_CONSOLE: window.window_ptr = make_scope<ui::console_window>(*driver_ptr->get_event_system(), driver_ptr); break;
        case BUILTIN_WINDOW_VIEWPORT: window.window_ptr = make_scope<ui::viewport>(*driver_ptr->get_event_system(), renderer, driver_ptr); break;
        case BUILTIN_WINDOW_SCENE_HIERARCHY: window.window_ptr = make_scope<ui::scene_hierarchy>(*driver_ptr->get_event_system(), driver_ptr); break;
        case BUILTIN_WINDOW_ASSET_BROWSER: window.window_ptr = make_scope<ui::asset_browser>(*driver_ptr->get_event_system(), driver_ptr); break;
        default:
          break;
      }
    }
  }

  void driver_ui::shutdown_builtin_windows() {
    CORE_LOG_DEBUG("Shutting down builtin UI windows...");
    for (size_t i = 0; i < NUM_BUILTIN_WINDOW_TYPES; ++i) {
      if (i == 0 || i == static_cast<size_t>(INVALID_WINDOW_TYPE)) {
        continue;
      }

      CORE_LOG_DEBUG("  - shutting down builtin window: {}", kBuiltinWindowNames[i]);
      auto& window = builtin_windows[i];
      window.window_ptr = nullptr;
    }
  }

  void driver_ui::shutdown_custom_windows() {
    CORE_LOG_DEBUG("Shutting down custom UI windows...");
    for (auto& [hash, window] : custom_windows) {
      CORE_LOG_DEBUG("  - shutting down custom window: {} [{}]", window.name, hash);
      window.window_ptr = nullptr;
    }
    custom_windows.clear();
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

  void driver_ui::render_builtin_windows() {
    for (size_t i = 0; i < NUM_BUILTIN_WINDOW_TYPES; ++i) {
      auto& window = builtin_windows[i];
      if (window.open && window.window_ptr != nullptr) {
        window.window_ptr->render();
      }
    }
  }

  void driver_ui::render_custom_windows() {
    for (auto& [hash, window] : custom_windows) {
      if (window.open && window.window_ptr != nullptr) {
        window.window_ptr->render();
      }
    }
  }

  void driver_ui::open_custom_window(const std::string_view name) {
    auto itr = custom_windows.find(FNV(name));
    if (itr == custom_windows.end()) {
      CORE_LOG_ERROR("Unknown custom UI window requested to open: {}", name);
      return;
    }

    if (itr->second.window_ptr == nullptr) {
      CORE_LOG_ERROR("Custom UI window '{}' appears to be unimplemented.", name);
      return;
    }
    itr->second.window_ptr->toggle_open();
    itr->second.open = true;
  }

  void driver_ui::close_custom_window(const std::string_view name) {
    auto itr = custom_windows.find(FNV(name));
    if (itr == custom_windows.end()) {
      CORE_LOG_ERROR("Unknown custom UI window requested to close: {}", name);
      return;
    }

    if (itr->second.window_ptr == nullptr) {
      CORE_LOG_ERROR("Custom UI window '{}' appears to be unimplemented.", name);
      return;
    }
    itr->second.window_ptr->toggle_close();
    itr->second.open = false;
  }

}  // namespace other