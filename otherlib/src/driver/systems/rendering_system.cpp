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

    configure_pipelines(kernel);

    driver_ui_ptr = make_scope<driver_ui>(&get_driver());
    driver_ui_ptr->initialize();

    auto open_windows = get_driver().configuration().get_value<std::vector<std::string>>("ui.open-windows", std::vector<std::string>{});
    for (const auto& window_name : open_windows) {
      value val = window_name;
      kernel->get_core_system<event_driver_system>().handle_open_ui_window_event(kernel, val);
    }

    auto& events = *get_driver().get_event_system();

    events.add_listener("viewport.resize", [this](const value& val) {
      handle_viewport_resize_event(val);
    });

    events.register_event("ls.windows");
    events.add_listener("ls.windows", [this](const value& data) { handle_ls_windows_event(&get_driver().get_kernel(), data); });
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

    render_data* data_ptr = nullptr;
    render_data prepared_data = {};
    if (active_scene != nullptr) {
      prepared_data = active_scene->prepare_render_data(viewport_size, asset_mgr);
      data_ptr = &prepared_data;
    }

    renderer_ptr->begin_frame(data_ptr);
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

  void rendering_system::configure_pipelines(driver_kernel* kernel) {
    OTHER_ASSERT(kernel != nullptr, "Driver kernel is null in configure_pipelines.");
    OTHER_ASSERT(renderer_ptr != nullptr, "Renderer is not initialized in configure_pipelines.");

    std::vector<std::string> pipeline_names = get_driver().get_config_value<std::vector<std::string>>("rendering.pipelines", {});
    if (pipeline_names.empty()) {
      pipeline_definition default_pipeline_def = get_default_instancing_pipeline();
      renderer_ptr->add_pipeline("Rendering Pipeline", default_pipeline_def);
      get_driver().add_rendering_pipeline_asset("default-rendering-pipeline", default_pipeline_def);
    } else {
      CORE_LOG_WARN("No rendering pipelines specified in configuration.");
    }
  }

  void rendering_system::handle_viewport_resize_event(const value& data) {
    if (data.type() != value_type::VEC2) {
      CORE_LOG_ERROR("Invalid data type for viewport resize event. Expected VEC2.");
      return;
    }
    viewport_size = data;
    get_driver().on_viewport_resize(viewport_size);
  }

  void rendering_system::handle_ls_windows_event(driver_kernel* kernel, const value& data) {
    auto& driver_ui_ptr = get_driver().get_ui();
    OTHER_ASSERT(driver_ui_ptr != nullptr, "Driver UI is not initialized.");

    std::vector<std::string> open_windows = driver_ui_ptr->get_open_window_names();
    std::vector<std::string> windows = std::span<const std::string_view>(driver_ui::kBuiltinWindowNames.data(), driver_ui::NUM_BUILTIN_WINDOW_TYPES).subspan(1) |
      std::views::transform([](const std::string_view& name) { return std::string(name); }) |
      std::views::filter([&open_windows](const std::string& name) { return std::ranges::find(open_windows, name) == open_windows.end(); }) |
      std::ranges::to<std::vector>();

    std::stringstream ss;
    ss << "Available Driver UI Windows:\n";
    for (const auto& window_name : open_windows) {
      ss << "  - " << window_name << " (open)\n";
    }
    for (const auto& window_name : windows) {
      ss << "  - " << window_name << "\n";
    }
    auto& events = get_driver().get_event_system();
    OTHER_ASSERT(events != nullptr, "Event system is not initialized.");
    events->trigger_event("console.output", ss.str());
  }

}  // namespace other