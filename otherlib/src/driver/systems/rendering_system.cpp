/**
 * \file driver/systems/rendering_system.cpp
 **/
#include "driver/systems/rendering_system.hpp"

#include "driver/driver.hpp"
#include "driver/systems/asset_system.hpp"
#include "driver/systems/scene_system.hpp"

namespace other {

  void rendering_system::initialize(driver_kernel* kernel) {
    renderer_ptr = make_scope<renderer>(get_driver().configuration());
    renderer_ptr->add_pipeline("Rendering Pipeline", get_default_instancing_pipeline());

    driver_ui_ptr = make_scope<driver_ui>(&get_driver());
    driver_ui_ptr->initialize();

    auto open_windows = get_driver().configuration().get_value<std::vector<std::string>>("ui.open-windows", std::vector<std::string>{});
    for (const auto& window_name : open_windows) {
      value val = window_name;
      kernel->get_core_system<event_driver_system>().handle_open_ui_window_event(kernel, val);
    }

    get_driver().get_event_system()->add_listener("viewport.resize", [this](const value& val) {
      handle_viewport_resize_event(val);
    });
  }

  void rendering_system::tick(driver_kernel* kernel, double dt) {
  }

  void rendering_system::shutdown(driver_kernel* kernel) {
    driver_ui_ptr->shutdown();
    driver_ui_ptr = nullptr;

    renderer_ptr->remove_pipeline("Rendering Pipeline");
  }

  void rendering_system::render(driver_kernel* kernel) {
    render_data data = {};
    auto window_size = renderer_ptr->get_window_size();
    if (viewport_size.x == 0 && viewport_size.y == 0) {
      viewport_size = window_size;
    }

    auto& scenes = kernel->get_core_system<scene_system>();
    auto* active_scene = scenes.get_active_scene();
    OTHER_ASSERT(kernel->has_core_system<asset_system>(), "Asset system is not available in driver kernel.");
    auto& asset_mgr = kernel->get_core_system<asset_system>().get_asset_manager();

    if (active_scene != nullptr) {
      data = active_scene->prepare_render_data(viewport_size, asset_mgr);
      renderer_ptr->begin_frame(&data);
    } else {
      renderer_ptr->begin_frame(nullptr);
    }
    renderer_ptr->render();

    render_ui(kernel);

    renderer_ptr->end_frame();
  }

  void rendering_system::render_ui(driver_kernel* kernel) {
    renderer_ptr->begin_ui_frame();
    driver_ui_ptr->render();
    /// render C# scripts
    // {
    //   PROFILE_SECTION("driver::render_ui--csharp-scripts");
    //   auto* env = subsystem<scripting_environment>::get();
    //   dotnet_object::invoke_state_function("UIWindowRegistry.RenderAll");
    // }
    renderer_ptr->end_ui_frame();
  }

  void rendering_system::open_ui_window(const std::string_view name) {
    if (driver_ui_ptr != nullptr) {
      driver_ui_ptr->open_window(name);
    } else {
      CORE_LOG_ERROR("Driver UI subsystem is not initialized. Cannot open UI window '{}'", name);
    }
  }

  void rendering_system::close_ui_window(const std::string_view name) {
    if (driver_ui_ptr != nullptr) {
      driver_ui_ptr->close_window(name);
    } else {
      CORE_LOG_ERROR("Driver UI subsystem is not initialized. Cannot close UI window '{}'", name);
    }
  }

  scope<renderer>& rendering_system::get_renderer() {
    OTHER_ASSERT(renderer_ptr != nullptr, "Renderer is not initialized in rendering system.");
    return renderer_ptr;
  }

  scope<driver_ui>& rendering_system::get_driver_ui() {
    OTHER_ASSERT(driver_ui_ptr != nullptr, "Driver UI is not initialized in rendering system.");
    return driver_ui_ptr;
  }

  void rendering_system::handle_viewport_resize_event(const value& data) {
    if (data.type() != value_type::VEC2) {
      CORE_LOG_ERROR("Invalid data type for viewport resize event. Expected VEC2.");
      return;
    }
    viewport_size = data;
    get_driver().on_viewport_resize(viewport_size);
  }

}  // namespace other