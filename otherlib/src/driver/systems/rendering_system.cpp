/**
 * \file driver/systems/rendering_system.cpp
 **/
#include "driver/systems/rendering_system.hpp"

#include <SDL3/SDL_dialog.h>

#include "driver/driver.hpp"
#include "driver/environment_registry.hpp"
#include "driver/systems/asset_system.hpp"
#include "driver/systems/scene_system.hpp"
#include "render/default_pass_executor_resolver.hpp"

#include "asset/pipelines/rendering_pipeline_pipeline.hpp"

namespace other {
  namespace detail {

    void file_dialog(SDL_FileDialogType type, SDL_Window* parent, rendering_system::file_dialog_callback_fn callback_fn, void* user_data, uint32_t props) {
      SDL_ShowFileDialogWithProperties(type, callback_fn, user_data, props);
    }

    render_graph::pass_executor make_noop(const pipeline_pass_definition&, render_pipeline*);
    render_graph::pass_executor make_draw_scene(const pipeline_pass_definition&, render_pipeline*);
    render_graph::pass_executor make_fullscreen_quad(const pipeline_pass_definition& def, render_pipeline* pl);
    render_graph::pass_executor make_compute_dispatch(const pipeline_pass_definition& def, render_pipeline* pl);
    render_graph::pass_executor make_debug_stream(const pipeline_pass_definition& def, render_pipeline* pl);

    void upload_camera_buffer_per_frame(render_pipeline& r, const render_data& d, resource_handle h);
    void upload_point_light_buffer_per_frame(render_pipeline& r, const render_data& d, resource_handle h);
    void upload_directional_light_buffer_per_frame(render_pipeline& r, const render_data& d, resource_handle h);
    void upload_model_buffer_per_draw(const render_data&, size_t draw_idx, std::span<uint8_t> data);
    void upload_material_buffer_per_draw(const render_data&, size_t draw_idx, std::span<uint8_t> data);
    void upload_bone_buffer_per_draw(const render_data&, size_t draw_idx, std::span<uint8_t> data);

    inline void no_op_upload_per_frame(render_pipeline&, const render_data&, resource_handle) {}

  }  // namespace detail

  void rendering_system::initialize(driver_kernel* kernel) {
    renderer_ptr = make_scope<renderer>(get_driver().configuration());
    register_builtin_resource_tags();
    register_builtin_render_executors();
    register_builtin_renderer_debug_streams();

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

    events.add_listener("rendering-pipeline.asset-loaded", [this](const value& data) { handle_rendering_pipeline_asset_loaded_event(&get_driver().get_kernel(), data); });
    events.add_listener("rendering-pipeline.asset-unloaded", [this](const value& data) { handle_rendering_pipeline_asset_unloaded_event(&get_driver().get_kernel(), data); });
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
        [this](scope<pass_executor_resolver> s) {
          renderer_ptr->initialize_pass_resolver(s.get());
          return 0;
        },
        [this](natural_t id) {
          renderer_ptr->initialize_pass_resolver(nullptr);
        },
        no_args(),
        interface_cardinality::SINGLE  //
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

    pass_resolver_ptr = nullptr;
    renderer_ptr = nullptr;
  }

  void rendering_system::render(driver_kernel* kernel) {
    render_data data = {};
    auto window_size = renderer_ptr->get_window_size();
    if (viewport_size.x == 0 && viewport_size.y == 0) {
      viewport_size = window_size;
    }

    OTHER_ASSERT(kernel->has_core_system<scene_system>(), "Scene system is not available in driver kernel.");
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

    if (data_ptr != nullptr) {
      auto& registry = renderer_ptr->get_debug_stream_registry();
      for (const auto& [name, def] : registry.entries()) {
        data_ptr->debug_data.configure_stream(def.name, def.element_size, def.max_per_frame);
      }
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

    auto& reg = renderer_ptr->get_binding_registry();
    reg.register_per_frame(resource_tag(resource_tag::kCameraTag), &detail::upload_camera_buffer_per_frame);
    reg.register_per_frame(resource_tag(resource_tag::kPointLightTag), &detail::upload_point_light_buffer_per_frame);
    reg.register_per_frame(resource_tag(resource_tag::kDirectionLightTag), &detail::upload_directional_light_buffer_per_frame);
    reg.register_per_frame(resource_tag(resource_tag::kScreenTag), &detail::no_op_upload_per_frame);
    reg.register_per_draw(resource_tag(resource_tag::kModelTag), &detail::upload_model_buffer_per_draw);
    reg.register_per_draw(resource_tag(resource_tag::kMaterialTag), &detail::upload_material_buffer_per_draw);
    reg.register_per_draw(resource_tag(resource_tag::kBoneTag), &detail::upload_bone_buffer_per_draw);
  }

  void rendering_system::register_builtin_render_executors() {
    OTHER_ASSERT(renderer_ptr != nullptr, "Renderer is null while registering executors!");

    auto& reg = renderer_ptr->get_executor_registry();
    reg.register_executor("noop", &detail::make_noop);
    reg.register_executor("draw_scene", &detail::make_draw_scene);
    reg.register_executor("fullscreen_quad", &detail::make_fullscreen_quad);
    reg.register_executor("compute_dispatch", &detail::make_compute_dispatch);
    reg.register_executor("debug_stream", &detail::make_debug_stream);
  }

  void rendering_system::register_builtin_renderer_debug_streams() {
    auto& reg = renderer_ptr->get_debug_stream_registry();
    // reg.register_stream("debug.lines", debug_stream_definition{
    //                                      .element_size = sizeof(debug_line),
    //                                      .max_per_frame = 4096,
    //                                      .draw_recipe = {
    //                                        .shader = "debug_line_shader",
    //                                        .topology = mesh::primitive_type::LINES,
    //                                        .vertex_layout = {
    //                                          vertex_attribute{ value_type::VEC3, "position", 0, 0 },
    //                                          vertex_attribute{ value_type::VEC3, "color", 1, 3 },
    //                                        },
    //                                      },
    //                                    });
    // reg.register_stream("debug.triangles", debug_stream_definition{
    //                                          .element_size = sizeof(debug_triangle),
    //                                          .max_per_frame = 2048,
    //                                          .draw_recipe = {
    //                                            .shader = "debug_tri_shader",
    //                                            .topology = mesh::primitive_type::TRIANGLES,
    //                                            .vertex_layout = {
    //                                              vertex_attribute{ value_type::VEC3, "position", 0, 0 },
    //                                              vertex_attribute{ value_type::VEC3, "color", 1, 3 },
    //                                            },
    //                                          },
    //                                        });

    // Load the recipe shaders once at registration time so the first frame
    // doesn't hit them lazily on draw_debug_stream.
    // for (auto* shader_name : { "debug_line_shader", "debug_tri_shader" }) {
    //   resource_handle h = shader::create(
    //     shader_name,
    //     std::format("resources/{}.vert", shader_name),
    //     std::format("resources/{}.frag", shader_name)
    //   );
    //   renderer_ptr->register_debug_stream_shader(shader_name, h);  // adds to map
    // }
  }

  void rendering_system::configure_pipelines(driver_kernel* kernel) {
    OTHER_ASSERT(kernel != nullptr, "Driver kernel is null in configure_pipelines.");
    OTHER_ASSERT(renderer_ptr != nullptr, "Renderer is not initialized in configure_pipelines.");

    std::string pass_resolver = get_driver().get_config_value<std::string>("rendering.pass-resolver", "default");
    if (pass_resolver == "default" && kernel->has_core_system<scripting_system>()) {
      pass_resolver_ptr = make_scope<default_pass_executor_resolver>(&kernel->get_core_system<scripting_system>());
    } else {
      OTHER_ASSERT(false, "Pass resolver plugin lookup not implemented yet!");
    }
    OTHER_ASSERT(pass_resolver_ptr != nullptr, "Pass resolver is null!");
    renderer_ptr->initialize_pass_resolver(pass_resolver_ptr.get());

    // {
    //   auto& ass = pending_rendering_pipeline_assets.emplace_back(pipeline_asset{
    //     .asset_id = 0,
    //     .definition = get_default_instancing_pipeline(),
    //   });
    //   natural_t default_pl = get_driver().add_rendering_pipeline_asset("default-rendering-pipeline", ass.definition);
    //   ass.asset_id = default_pl;
    // }

    std::vector<std::string> pipeline_paths = get_driver().get_config_value<std::vector<std::string>>("rendering.pipelines", {});
    if (pipeline_paths.size() > 0) {
      CORE_LOG_WARN("No rendering pipelines specified in configuration.");
      for (const auto& pipeline_path : pipeline_paths) {
        CORE_LOG_INFO("Adding rendering pipeline: {}", pipeline_path);
        auto& ass = pending_rendering_pipeline_assets.emplace_back(pipeline_asset{
          .asset_id = 0,
        });
        natural_t id = get_driver().begin_asset_load(filepath(pipeline_path));
        ass.asset_id = id;
      }
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
    std::vector<std::string> windows = driver_ui_ptr->get_available_window_names() |
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

  void rendering_system::handle_rendering_pipeline_asset_loaded_event(driver_kernel* kernel, const value& data) {
    OTHER_ASSERT(kernel != nullptr, "Kernel is null.");
    OTHER_ASSERT(renderer_ptr != nullptr, "Renderer is not initialized.");
    OTHER_ASSERT(data.type() == value_type::UINT64, "Invalid data type for rendering pipeline asset loaded event. Expected UINT64.");

    OTHER_ASSERT(kernel->has_core_system<asset_system>(), "Asset system is not initialized in the kernel.");
    auto& assets = kernel->get_core_system<asset_system>();

    natural_t asset_id = data;
    auto* asset_pipeline_ctx = assets.get_asset_pipeline_context(asset_id);
    OTHER_ASSERT(asset_pipeline_ctx != nullptr, "Asset pipeline context is not available for the given asset ID");

    auto itr = std::ranges::find(pending_rendering_pipeline_assets, asset_id, &pipeline_asset::asset_id);
    OTHER_ASSERT(itr != pending_rendering_pipeline_assets.end(), "Rendering pipeline asset loaded event for unknown asset ID: {}", asset_id);

    rendering_pipeline_pipeline* pl = dynamic_cast<rendering_pipeline_pipeline*>(asset_pipeline_ctx->pipeline.get());
    OTHER_ASSERT(pl != nullptr, "Rendering pipeline pipeline is not available for the given asset ID");

    itr->definition = pl->definition;

    CORE_LOG_INFO("Adding Rendering Pipeline: {}", itr->definition.name);
    renderer_ptr->add_pipeline(itr->definition.name, itr->definition);
    rendering_pipeline_assets.push_back(*itr);
    pending_rendering_pipeline_assets.erase(itr);
  }

  void rendering_system::handle_rendering_pipeline_asset_unloaded_event(driver_kernel* kernel, const value& data) {
    OTHER_ASSERT(kernel != nullptr, "Kernel is null.");
    OTHER_ASSERT(renderer_ptr != nullptr, "Renderer is not initialized.");
    OTHER_ASSERT(data.type() == value_type::UINT64, "Invalid data type for rendering pipeline asset unloaded event. Expected UINT64.");

    natural_t asset_id = data;
    auto itr = std::ranges::find(rendering_pipeline_assets, asset_id, &pipeline_asset::asset_id);
    OTHER_ASSERT(itr != rendering_pipeline_assets.end(), "Rendering pipeline asset unloaded event for unknown asset ID: {}", asset_id);

    renderer_ptr->remove_pipeline(itr->definition.name);
    rendering_pipeline_assets.erase(itr);
  }

  namespace detail {

    render_graph::pass_executor make_noop(const pipeline_pass_definition&, render_pipeline*) {
      return [](pass_context& ctx) {
      };
    }

    render_graph::pass_executor make_draw_scene(const pipeline_pass_definition&, render_pipeline*) {
      return [](pass_context& ctx) {
        ctx.draw_stream();
      };
    }

    render_graph::pass_executor make_fullscreen_quad(const pipeline_pass_definition& def, render_pipeline* pl) {
      resource_handle quad = pl->get_quad_mesh_handle();
      return [quad, uniforms = def.executor.uniforms, pass_name = def.name, pl](pass_context& ctx) {
        auto* sh = pl->get_pass_shader(pass_name);
        OTHER_ASSERT(sh, "fullscreen_quad: no shader bound for pass '{}'", pass_name);
        render_pipeline::apply_uniforms(*sh, uniforms);
        ctx.draw_quad();
      };
    }

    render_graph::pass_executor make_compute_dispatch(const pipeline_pass_definition& def, render_pipeline* pl) {
      const auto& params = def.executor.params;
      auto groups_it = params.find("groups");
      OTHER_ASSERT(groups_it != params.end(), "compute_dispatch: pass '{}' missing 'groups' param", def.name);
      glm::vec3 groups = groups_it->second;

      shader::compute_barrier_type barrier = shader::compute_barrier_type::NONE;
      if (auto b_it = params.find("barrier"); b_it != params.end()) {
        barrier = compute_barrier_type_from_string(b_it->second);
      }
      return [g = groups, b = barrier, pass_name = def.name](pass_context& ctx) {
        auto* sh = ctx.shader_for_pass();
        OTHER_ASSERT(sh != nullptr, "compute_dispatch: no shader bound for pass '{}'", pass_name);
        sh->bind();
        ctx.dispatch(glm::uvec3(g), b);
      };
    };

    render_graph::pass_executor make_debug_stream(const pipeline_pass_definition& def, render_pipeline* pl) {
      const auto& params = def.executor.params;
      auto stream_it = params.find("stream");
      OTHER_ASSERT(stream_it != params.end(), "debug_stream: pass '{}' missing 'stream' param", def.name);
      std::string stream_name = stream_it->second;

      return [stream_name = std::move(stream_name), pass_name = def.name](pass_context& ctx) {
        auto& streams = ctx.get_frame_data().debug_data;
        auto* stream = ctx.get_renderer().get_debug_stream_registry().find(stream_name);
        if (stream == nullptr) {
          CORE_LOG_ERROR("Debug stream '{}' not found for pass '{}'", stream_name, pass_name);
          return;
        }
        ctx.draw_debug_stream(stream_name, *stream, streams.view(stream_name), streams.count(stream_name));
      };
    }

    void upload_camera_buffer_per_frame(render_pipeline& r, const render_data& d, resource_handle h) {
      if (!d.primary_camera) {
        return;
      }

      auto gpu = d.primary_camera->to_gpu_data();
      r.upload_buffer(h, &gpu, sizeof(gpu));
    }

    void upload_point_light_buffer_per_frame(render_pipeline& r, const render_data& d, resource_handle h) {
      gpu::point_light_buffer buf{};
      for (size_t i = 0; i < d.point_lights.size() && i < gpu::kMaxPointLights; ++i) {
        buf.lights[i] = d.point_lights[i];
      }
      r.upload_to_handle(h, &buf, sizeof(gpu::point_light_buffer));
    }

    void upload_directional_light_buffer_per_frame(render_pipeline& r, const render_data& d, resource_handle h) {
      gpu::directional_light_buffer dir_light_buffer_data;
      for (size_t i = 0; i < d.ambient_lights.size() && i < gpu::kMaxDirectionalLights; ++i) {
        dir_light_buffer_data.lights[i] = d.ambient_lights[i];
      }
      r.upload_to_handle(h, &dir_light_buffer_data, sizeof(gpu::directional_light_buffer));
    }

    void upload_model_buffer_per_draw(const render_data& d, size_t draw_idx, std::span<uint8_t> data) {
      OTHER_ASSERT(draw_idx < d.draw_calls.size(), "Draw index {} out of range for draw calls of size {}", draw_idx, d.draw_calls.size());
      OTHER_ASSERT(data.size() == sizeof(gpu::model_matrix_buffer), "Data span size {} does not match expected size {}", data.size(), sizeof(gpu::model_matrix_buffer));

      const auto& models = d.model_buffers[draw_idx];
      std::span bytes{ reinterpret_cast<const uint8_t*>(&models), sizeof(gpu::model_matrix_buffer) };
      std::ranges::copy(bytes, data.begin());
    }

    void upload_material_buffer_per_draw(const render_data& d, size_t draw_idx, std::span<uint8_t> data) {
      OTHER_ASSERT(draw_idx < d.draw_calls.size(), "Draw index {} out of range for draw calls of size {}", draw_idx, d.draw_calls.size());
      OTHER_ASSERT(data.size() == sizeof(gpu::graphics_material_buffer), "Data span size {} does not match expected size {}", data.size(), sizeof(gpu::graphics_material_buffer));

      const auto& materials = d.material_buffers[draw_idx];
      std::span bytes{ reinterpret_cast<const uint8_t*>(&materials), sizeof(gpu::graphics_material_buffer) };
      std::ranges::copy(bytes, data.begin());
    }

    void upload_bone_buffer_per_draw(const render_data& d, size_t draw_idx, std::span<uint8_t> data) {
      OTHER_ASSERT(draw_idx < d.draw_calls.size(), "Draw index {} out of range for draw calls of size {}", draw_idx, d.draw_calls.size());
      OTHER_ASSERT(data.size() == sizeof(gpu::bone_matrix_buffer), "Data span size {} does not match expected size {}", data.size(), sizeof(gpu::bone_matrix_buffer));

      const auto& bones = d.bone_buffers[draw_idx];
      std::span bytes{ reinterpret_cast<const uint8_t*>(&bones), sizeof(gpu::bone_matrix_buffer) };
      std::ranges::copy(bytes, data.begin());
    }

  }  // namespace detail
}  // namespace other