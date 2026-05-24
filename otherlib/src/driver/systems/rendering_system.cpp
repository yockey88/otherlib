/**
 * \file driver/systems/rendering_system.cpp
 **/
#include "driver/systems/rendering_system.hpp"

#include <SDL3/SDL_dialog.h>

#include "renderer/default_pass_executor_resolver.hpp"

#include "driver/driver.hpp"
#include "driver/environment_registry.hpp"
#include "driver/systems/asset_system.hpp"
#include "driver/systems/scene_system.hpp"

namespace other {
  namespace detail {

    void file_dialog(SDL_FileDialogType type, SDL_Window* parent, rendering_system::file_dialog_callback_fn callback_fn, void* user_data, uint32_t props) {
      SDL_ShowFileDialogWithProperties(type, callback_fn, user_data, props);
    }

    render_graph::pass_executor make_noop(const pipeline_pass_definition&, render_pipeline*);
    render_graph::pass_executor make_draw_scene(const pipeline_pass_definition&, render_pipeline*);
    render_graph::pass_executor make_fullscreen_quad(const pipeline_pass_definition& def, render_pipeline* pl);
    render_graph::pass_executor make_compute_dispatch(const pipeline_pass_definition& def, render_pipeline* pl);

  }  // namespace detail

  void rendering_system::initialize(driver_kernel* kernel) {
    renderer_ptr = make_scope<renderer>(get_driver().configuration());
    register_builtin_resource_tags();
    register_builtin_render_executors();

    configure_pipelines(kernel);

    driver_ui_ptr = make_scope<driver_ui>(&get_driver());
    driver_ui_ptr->initialize();

    auto open_windows = get_driver().configuration().get_value<std::vector<std::string>>("ui.open-windows", std::vector<std::string>{});
    for (const auto& window_name : open_windows) {
      value val = window_name;
      sibling<event_driver_system>(*kernel).handle_open_ui_window_event(kernel, val);
    }

    auto& events = *get_driver().get_event_system();

    events.add_listener("viewport.resize", [this](const value& val) { handle_viewport_resize_event(val); });

    events.register_event("ls.windows");
    events.add_listener("ls.windows", [this](const value& data) { handle_ls_windows_event(&get_driver().get_kernel(), data); });
  }

  void rendering_system::late_initialize(driver_kernel* kernel) {
    auto register_interfaces_in_registry = [this](environment_registry& reg) {
      reg.register_interface<ui_window>(
        [this](scope<ui_window> s, plugin_param_view params) {
          auto name = params.get_or("name", "Unnamed Window");
          return driver_ui_ptr->register_window(name, std::move(s));
        },
        [this](natural_t id) { driver_ui_ptr->unregister_window(id); },
        ui_window_args(get_driver().get_event_system().get()),
        interface_cardinality::MULTIPLE  // don't want too many
      );
      reg.register_interface<pass_executor_resolver>(
        [this](scope<pass_executor_resolver> s, plugin_param_view params) {
          opt<std::string_view> pipeline_name = params.get("pipeline");
          opt<std::string_view> pass_name = params.get("pass");
          if (!pipeline_name.has_value() || !pass_name.has_value()) {
            // clang-format off
            CORE_LOG_ERROR("Pass executor interface can only be attached to a specific pipeline and specific pass! pipeline: {}, pass: {}",
                            pipeline_name.has_value() ? *pipeline_name : "<empty>", pass_name.has_value() ? *pass_name : "<empty>");
            // clang-format on
          }
          return 0;
        },
        [this](natural_t id) {},
        no_args(),
        interface_cardinality::MULTIPLE
      );
    };
    register_interfaces_in_registry(kernel->driver_registry());
    register_interfaces_in_registry(kernel->project_registry());
  }

  void rendering_system::tick(driver_kernel* kernel, double dt) {
  }

  void rendering_system::shutdown(driver_kernel* kernel) {
    OTHER_ASSERT(driver_ui_ptr != nullptr, "Driver UI is not initialized in rendering system shutdown.");
    driver_ui_ptr->shutdown();
    driver_ui_ptr = nullptr;

    renderer_ptr->remove_pipeline("Rendering Pipeline");
    pass_resolver_ptr = nullptr;
    renderer_ptr = nullptr;
  }

  void rendering_system::render(driver_kernel* kernel) {
    render_data data = {};
    auto window_size = renderer_ptr->get_window_size();
    if (viewport_size.x == 0 && viewport_size.y == 0) {
      viewport_size = window_size;
    }

    auto& scenes = sibling<scene_system>(*kernel);
    auto* active_scene = scenes.get_active_scene();
    OTHER_ASSERT(kernel->has_core_system<asset_system>(), "Asset system is not available in driver kernel.");
    auto& asset_mgr = sibling<asset_system>(*kernel).get_asset_manager();

    render_data* data_ptr = nullptr;
    render_data prepared_data = {};
    if (active_scene != nullptr) {
      prepared_data = active_scene->prepare_render_data(viewport_size, asset_mgr);
      data_ptr = &prepared_data;
    }

    renderer_ptr->begin_frame(data_ptr);
    renderer_ptr->render();

    renderer_ptr->begin_ui_frame();
    driver_ui_ptr->render();
    renderer_ptr->end_ui_frame();

    renderer_ptr->end_frame();
  }

  void rendering_system::open_ui_window(const std::string_view name) {
    OTHER_ASSERT(driver_ui_ptr != nullptr, "Driver UI subsystem is not initialized. Cannot open UI window '{}'", name);
    driver_ui_ptr->open_window(name);
  }

  void rendering_system::close_ui_window(const std::string_view name) {
    OTHER_ASSERT(driver_ui_ptr != nullptr, "Driver UI subsystem is not initialized. Cannot close UI window '{}'", name);
    driver_ui_ptr->close_window(name);
  }

  ui::menu rendering_system::build_menu(const std::string_view name, const sol::table& menu_table) {
    ui::menu menu = {
      .name = std::string{ name },
      .sub_menus = {},
      .items = {},
    };

    for (const auto& pair : menu_table) {
      sol::table item = pair.second.as<sol::table>();

      std::string item_name = item["Name"];
      sol::optional<sol::function> action_fn = item["Action"];
      sol::optional<sol::table> sub_items = item["SubItems"];

      if (sub_items.has_value()) {
        menu.sub_menus.push_back(build_menu(item_name, sub_items.value()));
      } else if (action_fn.has_value()) {
        menu.items.push_back(build_menu_item(item_name, action_fn.value()));
      } else {
        CORE_LOG_WARN("Menu item '{}' in menu '{}' has no action or sub-items defined.", item_name, name);
      }
    }
    return menu;
  }

  ui::menu_item rendering_system::build_menu_item(const std::string_view name, sol::function action_fn) {
    ui::menu_item item = {
      .name = std::string(name),
      .action = std::nullopt,
    };
    if (action_fn.valid()) {
      item.action = action();
      item.action->set_callback(make_ref<lua_callback>(action_fn, std::string(name) + "." + "action"));
    }
    return item;
  }

  scope<renderer>& rendering_system::get_renderer() {
    OTHER_ASSERT(renderer_ptr != nullptr, "Renderer is not initialized in rendering system.");
    return renderer_ptr;
  }

  scope<driver_ui>& rendering_system::get_driver_ui() {
    OTHER_ASSERT(driver_ui_ptr != nullptr, "Driver UI is not initialized in rendering system.");
    return driver_ui_ptr;
  }

  void rendering_system::show_open_file_dialog(file_dialog_callback_fn callback_fn, void* user_data, uint32_t props) {
    detail::file_dialog(SDL_FILEDIALOG_OPENFILE, nullptr, callback_fn, user_data, props);
  }

  void rendering_system::show_open_folder_dialog(file_dialog_callback_fn callback_fn, void* user_data, uint32_t props) {
    detail::file_dialog(SDL_FILEDIALOG_OPENFOLDER, nullptr, callback_fn, user_data, props);
  }

  void rendering_system::show_save_file_dialog(file_dialog_callback_fn callback_fn, void* user_data, uint32_t props) {
    detail::file_dialog(SDL_FILEDIALOG_SAVEFILE, nullptr, callback_fn, user_data, props);
  }

  void rendering_system::register_builtin_resource_tags() {
    OTHER_ASSERT(renderer_ptr != nullptr, "Renderer is not initialized in register_builtin_resource_tags.");

    auto& reg = renderer_ptr->get_tag_registry();
    reg.register_binder(resource_tag(resource_tag::kCameraTag), [](render_pipeline& r, const render_data& d, resource_handle h) {
      if (!d.primary_camera) {
        return;
      }

      auto gpu = d.primary_camera->to_gpu_data();
      r.upload_buffer(h, &gpu, sizeof(gpu));
    });

    reg.register_binder(resource_tag(resource_tag::kModelTag), [](render_pipeline& r, const render_data& d, resource_handle h) {
      // r.upload_buffer(h, d.model_buffers.data(), d.model_buffers.size() * sizeof(gpu::model_matrix_buffer));
    });

    reg.register_binder(resource_tag(resource_tag::kMaterialTag), [](render_pipeline& r, const render_data& d, resource_handle h) {
      // r.upload_buffer(h, d.material_buffers.data(), d.material_buffers.size() * sizeof(gpu::material_buffer));
    });

    reg.register_binder(resource_tag(resource_tag::kBoneTag), [](render_pipeline& r, const render_data& d, resource_handle h) {
      if (d.bone_buffers.empty()) {
        return;
      }
      // r.upload_buffer(h, d.bone_buffers.data(), d.bone_buffers.size() * sizeof(gpu::bone_matrix_buffer));
    });

    reg.register_binder(resource_tag(resource_tag::kPointLightTag), [](render_pipeline& r, const render_data& d, resource_handle h) {
      gpu::point_light_buffer buf{};
      for (size_t i = 0; i < d.point_lights.size() && i < gpu::kMaxPointLights; ++i) {
        buf.lights[i] = d.point_lights[i];
      }
      r.upload_to_handle(h, &buf, sizeof(gpu::point_light_buffer));
    });

    reg.register_binder(resource_tag(resource_tag::kDirectionLightTag), [](render_pipeline& r, const render_data& d, resource_handle h) {
      gpu::directional_light_buffer dir_light_buffer_data;
      for (size_t i = 0; i < d.ambient_lights.size() && i < gpu::kMaxDirectionalLights; ++i) {
        dir_light_buffer_data.lights[i] = d.ambient_lights[i];
      }
      r.upload_to_handle(h, &dir_light_buffer_data, sizeof(gpu::directional_light_buffer));
    });

    // screen is simply a marker tag, render_pipeline tracks screen_texture_handle separately for to-screen blit
    reg.register_binder(resource_tag(resource_tag::kScreenTag), [](render_pipeline&, const render_data&, resource_handle) {});
  }

  void rendering_system::register_builtin_render_executors() {
    OTHER_ASSERT(renderer_ptr != nullptr, "Renderer is null while registering executors!");

    auto& reg = renderer_ptr->get_executor_registry();
    reg.register_executor("noop", &detail::make_noop);
    reg.register_executor("draw_scene", &detail::make_draw_scene);
    reg.register_executor("fullscreen_quad", &detail::make_fullscreen_quad);
    reg.register_executor("compute_dispatch", &detail::make_compute_dispatch);
  }

  void rendering_system::configure_pipelines(driver_kernel* kernel) {
    OTHER_ASSERT(kernel != nullptr, "Driver kernel is null in configure_pipelines.");
    OTHER_ASSERT(renderer_ptr != nullptr, "Renderer is not initialized in configure_pipelines.");

    std::string pass_resolver = get_driver().get_config_value<std::string>("rendering.pass-resolver", "default");
    if (pass_resolver == "default") {
      pass_resolver_ptr = make_scope<default_pass_executor_resolver>(kernel->has_core_system<scripting_system>());
    } else {
      OTHER_ASSERT(false, "Pass resolver plugin lookup not implemented yet!");
    }
    OTHER_ASSERT(pass_resolver_ptr != nullptr, "Pass resolver is null!");
    renderer_ptr->initialize_pass_resolver(pass_resolver_ptr.get());

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

  namespace detail {

    render_graph::pass_executor make_noop(const pipeline_pass_definition&, render_pipeline*) {
      return [](renderer&, render_graph::node*, void*) {
      };
    }

    render_graph::pass_executor make_draw_scene(const pipeline_pass_definition&, render_pipeline*) {
      return [](renderer& r, render_graph::node* node, void*) {
        r.execute_draw_calls(node);
      };
    }

    render_graph::pass_executor make_fullscreen_quad(const pipeline_pass_definition& def, render_pipeline* pl) {
      resource_handle quad = pl->get_quad_mesh_handle();
      return [quad, uniforms = def.executor.uniforms, pass_name = def.name, pl](renderer& r, render_graph::node*, void*) {
        auto* sh = pl->get_pass_shader(pass_name);
        OTHER_ASSERT(sh, "fullscreen_quad: no shader bound for pass '{}'", pass_name);
        render_pipeline::apply_uniforms(*sh, uniforms);
        r.get_resource<mesh>(quad).draw();
      };
    }

    render_graph::pass_executor make_compute_dispatch(const pipeline_pass_definition& def, render_pipeline* pl) {
      // params: { "groups" = [int, int, int] }
      const auto& params = def.executor.params;
      auto it = params.find("groups");
      OTHER_ASSERT(it != params.end(), "compute_dispatch: pass '{}' missing 'groups' param", def.name);

      glm::vec3 g = it->second;
      return [g, pass_name = def.name, pl](renderer& r, render_graph::node*, void*) {
        auto* sh = pl->get_pass_shader(pass_name);
        OTHER_ASSERT(sh, "compute_dispatch: no shader for pass '{}'", pass_name);
        r.dispatch_compute(sh, g.x, g.y, g.z);
      };
    };

  }  // namespace detail
}  // namespace other