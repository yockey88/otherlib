/**
 * \file driver/systems/rendering_system.cpp
 **/
#include "driver/systems/rendering_system.hpp"

#include <SDL3/SDL_dialog.h>

#include "gpu_resource/framebuffer.hpp"
#include "renderer/gpu_structs.hpp"

#include "driver/driver.hpp"
#include "driver/environment_registry.hpp"
#include "driver/systems/asset_system.hpp"
#include "driver/systems/scene_system.hpp"
#include "driver/systems/scripting_system.hpp"
#include "render/default_pass_executor_resolver.hpp"
#include "ui/inspector_widgets.hpp"
#include "ui/script/script_field_ui.hpp"

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
    render_graph::pass_executor make_window_sized_compute_dispatch(const pipeline_pass_definition& def, render_pipeline* pl);
    render_graph::pass_executor make_voxelize(const pipeline_pass_definition& def, render_pipeline* pl);
    render_graph::pass_executor make_generate_mipmaps(const pipeline_pass_definition& def, render_pipeline* pl);
    render_graph::pass_executor make_downsample_chain(const pipeline_pass_definition& def, render_pipeline* pl);
    render_graph::pass_executor make_debug_overlay(const pipeline_pass_definition& def, render_pipeline* pl);
    render_graph::pass_executor make_debug_meshes(const pipeline_pass_definition& def, render_pipeline* pl);

    void upload_camera_buffer_per_frame(render_pipeline& r, const render_data& d, resource_handle h);
    void upload_light_buffer_per_frame(render_pipeline& r, const render_data& d, resource_handle h);
    void upload_simulation_environment_buffer_per_frame(render_pipeline& r, const render_data& d, resource_handle h);
    void upload_model_buffer_per_draw(const render_data&, size_t draw_idx, std::span<uint8_t> data);
    void upload_material_buffer_per_draw(const render_data&, size_t draw_idx, std::span<uint8_t> data);
    void upload_bone_buffer_per_draw(const render_data&, size_t draw_idx, std::span<uint8_t> data);

    inline void no_op_upload_per_frame(render_pipeline&, const render_data&, resource_handle) {}

  }  // namespace detail

  void rendering_system::initialize(driver_kernel* kernel) {
    PROFILE_SECTION("rendering_system::initialize");
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

    events.register_event("ls.windows");
    events.add_listener("ls.windows", [this](const value& data) { handle_ls_windows_event(&get_driver().get_kernel(), data); });

    events.add_listener("rendering-pipeline.asset-loaded", [this](const value& data) { handle_rendering_pipeline_asset_loaded_event(&get_driver().get_kernel(), data); });
    events.add_listener("rendering-pipeline.asset-unloaded", [this](const value& data) { handle_rendering_pipeline_asset_unloaded_event(&get_driver().get_kernel(), data); });

    auto& field_editors = get_driver().get_field_editors();
    field_editors.register_editor<bool>([](const std::string_view l, void* d, const ui::field_context& c) { return ui::inspector::property_bool(l, *static_cast<bool*>(d)); });
    field_editors.register_editor<int8_t>([](const std::string_view l, void* d, const ui::field_context& c) { return ui::inspector::property_int8(l, *static_cast<int8_t*>(d)); });
    field_editors.register_editor<int16_t>([](const std::string_view l, void* d, const ui::field_context& c) { return ui::inspector::property_int16(l, *static_cast<int16_t*>(d)); });
    field_editors.register_editor<int32_t>([](const std::string_view l, void* d, const ui::field_context& c) { return ui::inspector::property_int32(l, *static_cast<int32_t*>(d)); });
    field_editors.register_editor<int64_t>([](const std::string_view l, void* d, const ui::field_context& c) { return ui::inspector::property_int64(l, *static_cast<int64_t*>(d)); });
    field_editors.register_editor<uint8_t>([](const std::string_view l, void* d, const ui::field_context& c) { return ui::inspector::property_uint8(l, *static_cast<uint8_t*>(d)); });
    field_editors.register_editor<uint16_t>([](const std::string_view l, void* d, const ui::field_context& c) { return ui::inspector::property_uint16(l, *static_cast<uint16_t*>(d)); });
    field_editors.register_editor<uint32_t>([](const std::string_view l, void* d, const ui::field_context& c) { return ui::inspector::property_uint32(l, *static_cast<uint32_t*>(d)); });
    field_editors.register_editor<uint64_t>([](const std::string_view l, void* d, const ui::field_context& c) { return ui::inspector::property_uint64(l, *static_cast<uint64_t*>(d)); });
    field_editors.register_editor<float>([](const std::string_view l, void* d, const ui::field_context& c) {
      float& v = *static_cast<float*>(d);
      if (c.flags.is_color) {
        ui::inspector::begin_property_row(c.flags.display_name.empty() ? l : c.flags.display_name);
        std::string id = std::format("##{}", l);
        bool ch = ImGui::ColorEdit3(id.c_str(), &v, ImGuiColorEditFlags_Float);
        ui::inspector::end_property_row();
        return ch;
      }

      if (c.flags.has_range) {
        ui::inspector::begin_property_row(l);
        std::string id = std::format("##{}", l);
        bool ch = ImGui::SliderFloat(id.c_str(), &v, c.flags.range.x, c.flags.range.y);
        ui::inspector::end_property_row();
        return ch;
      }

      float speed = c.flags.speed.value_or(0.01f);
      return ui::inspector::property_float(l, v, speed);
    });

    field_editors.register_editor<double>([](const std::string_view label, void* d, const ui::field_context& c) {
      double& v = *static_cast<double*>(d);
      float f = static_cast<float>(v);
      if (c.flags.has_range) {
        ui::inspector::begin_property_row(label);

        std::string id = std::format("##{}", label);
        bool ch = ImGui::SliderFloat(id.c_str(), &f, c.flags.range.x, c.flags.range.y);

        ui::inspector::end_property_row();
        if (ch) {
          v = f;
        }

        return ch;
      }

      float speed = c.flags.speed.value_or(0.01f);
      if (ui::inspector::property_float(label, f, speed)) {
        v = static_cast<double>(f);
        return true;
      }

      return false;
    });

    field_editors.register_editor<glm::vec2>([](auto l, void* d, const ui::field_context& c) { return ui::inspector::property_vec2(l, *static_cast<glm::vec2*>(d), c.flags.speed.value_or(0.01f)); });
    field_editors.register_editor<glm::vec3>([](auto l, void* d, const ui::field_context& c) { return ui::inspector::property_vec3(l, *static_cast<glm::vec3*>(d), c.flags.speed.value_or(0.01f)); });
    field_editors.register_editor<glm::vec4>([](auto l, void* d, const ui::field_context& c) { return ui::inspector::property_vec4(l, *static_cast<glm::vec4*>(d), c.flags.speed.value_or(0.01f)); });
    field_editors.register_editor<glm::mat3>([](auto l, void* d, const ui::field_context& c) { return ui::inspector::property_mat3(l, *static_cast<glm::mat3*>(d), c.flags.speed.value_or(0.01f)); });
    field_editors.register_editor<glm::mat4>([](auto l, void* d, const ui::field_context& c) { return ui::inspector::property_mat4(l, *static_cast<glm::mat4*>(d), c.flags.speed.value_or(0.01f)); });
    field_editors.register_editor<glm::quat>([](const std::string_view label, void* d, const ui::field_context& c) {
      auto& q = *static_cast<glm::quat*>(d);

      glm::vec3 euler = glm::degrees(glm::eulerAngles(q));
      if (ui::inspector::property_vec3(label, euler, c.flags.speed.value_or(0.01f))) {
        q = glm::quat(glm::radians(euler));
        return true;
      }
      return false;
    });
    field_editors.register_editor<std::string>([](const std::string_view label, void* d, const ui::field_context& c) {
      auto& s = *static_cast<std::string*>(d);

      char buf[256];
      std::strncpy(buf, s.c_str(), sizeof(buf));
      buf[sizeof(buf) - 1] = '\0';

      if (ui::inspector::property_text(label, buf, sizeof(buf))) {
        s = std::string(buf);
        return true;
      }
      return false;
    });
    field_editors.register_editor<orthonormal_basis>([](const std::string_view, void*, const ui::field_context&) {
      return false;  // intentional no-op, preserves transform.local_basis behaviour
    });

    auto scalar = [](ImGuiDataType ig) {
      return [ig](const std::string_view label, void* d, const ui::field_context&) {
        std::string id = std::format("{}##scalar", label);
        return ImGui::DragScalar(id.c_str(), ig, d, 0.1f, nullptr, nullptr, nullptr, 0);
      };
    };
    field_editors.register_value_editor(value_type::INT8, scalar(ImGuiDataType_S8), true);
    field_editors.register_value_editor(value_type::INT16, scalar(ImGuiDataType_S16), true);
    field_editors.register_value_editor(value_type::INT32, scalar(ImGuiDataType_S32), true);
    field_editors.register_value_editor(value_type::INT64, scalar(ImGuiDataType_S64), true);
    field_editors.register_value_editor(value_type::UINT8, scalar(ImGuiDataType_U8), true);
    field_editors.register_value_editor(value_type::UINT16, scalar(ImGuiDataType_U16), true);
    field_editors.register_value_editor(value_type::UINT32, scalar(ImGuiDataType_U32), true);
    field_editors.register_value_editor(value_type::UINT64, scalar(ImGuiDataType_U64), true);

    field_editors.register_editor<point_light>([](const std::string_view label, void* d, const ui::field_context& ctx) {
      point_light& pl = *reinterpret_cast<point_light*>(d);
      bool modified = false;

      ImGui::Text("%s", label.data());

      const std::string id = std::format("{}##point_light", label);
      ImGui::PushID(id.c_str());

      modified |= ui::inspector::property_vec3("Position", pl.position, ctx.flags.speed.value_or(0.01));
      modified |= ui::inspector::property_vec4("Color", pl.color, ctx.flags.speed.value_or(0.01));

      ImGui::PopID();
      return modified;
    });
    field_editors.register_editor<direction_light>([](const std::string_view label, void* d, const ui::field_context& ctx) {
      direction_light& dl = *reinterpret_cast<direction_light*>(d);
      bool modified = false;

      ImGui::Text("%s", label.data());

      const std::string id = std::format("{}##directional_light", label);
      ImGui::PushID(id.c_str());

      modified |= ui::inspector::property_vec3("Direction", dl.direction, ctx.flags.speed.value_or(0.01));
      modified |= ui::inspector::property_vec4("Color", dl.color, ctx.flags.speed.value_or(0.01));

      ImGui::PopID();
      return modified;
    });
  }

  void rendering_system::late_initialize(driver_kernel* kernel) {
    PROFILE_SECTION("rendering_system::late_initialize");
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

  void rendering_system::on_driver_ready(driver_kernel* kernel) {
    OTHER_ASSERT(renderer_ptr != nullptr, "Renderer is not initialized in rendering system on_driver_ready.");
    PROFILE_SECTION("rendering_system::on_driver_ready");

    {
      ui::menu view = {
        .name = "View",
        .dynamic_sub_menus = [this]() { get_driver().get_ui()->available_ui_window_menu(); }
      };
      get_driver().get_ui()->register_main_menu_bar_menu(view);
    }
  }

  void rendering_system::tick(driver_kernel* kernel, double dt) {
    OTHER_ASSERT(renderer_ptr != nullptr, "Renderer is not initialized in rendering system tick.");
    PROFILE_SECTION("rendering_system::tick");
  }

  void rendering_system::shutdown(driver_kernel* kernel) {
    OTHER_ASSERT(driver_ui_ptr != nullptr, "Driver UI is not initialized in rendering system shutdown.");
    PROFILE_SECTION("rendering_system::shutdown");

    renderer_ptr->shutdown();

    driver_ui_ptr->shutdown();
    driver_ui_ptr = nullptr;

    pass_resolver_ptr = nullptr;
    renderer_ptr = nullptr;
  }

  void rendering_system::render(driver_kernel* kernel) {
    PROFILE_SECTION("rendering_system::render");

    OTHER_ASSERT(kernel->has_core_system<scene_system>(), "Scene system is not available in driver kernel.");
    auto& scenes = sibling<scene_system>(*kernel);
    auto* active_scene = scenes.get_active_scene();

    OTHER_ASSERT(kernel->has_core_system<asset_system>(), "Asset system is not available in driver kernel.");
    auto& asset_mgr = sibling<asset_system>(*kernel).get_asset_manager();

    render_data* data_ptr = nullptr;
    render_data prepared_data = {};
    if (active_scene != nullptr) {
      prepared_data = active_scene->prepare_render_data(asset_mgr);
      auto& reg = renderer_ptr->get_debug_stream_registry();
      prepared_data.debug_data.configure_streams(renderer_ptr.get(), reg);
      data_ptr = &prepared_data;
    }

    {
      PROFILE_SECTION("rendering_system::render--frame");
      renderer_ptr->begin_frame(data_ptr);
      get_driver().on_begin_frame(data_ptr);

      renderer_ptr->render(viewports);
      get_driver().on_render();
      get_driver().on_debug_render(data_ptr);

      const bool ui_enabled = get_driver().get_config_value<bool>("ui.enable", true);
      if (ui_enabled) {
        renderer_ptr->begin_ui_frame();
        driver_ui_ptr->render();
        get_driver().on_ui_render();
        renderer_ptr->end_ui_frame();
      }

      renderer_ptr->end_frame();
    }

    if (data_ptr != nullptr) {
      data_ptr->debug_data.clear();
    }
  }

  void rendering_system::open_ui_window(const std::string_view name) {
    OTHER_ASSERT(driver_ui_ptr != nullptr, "Driver UI subsystem is not initialized. Cannot open UI window '{}'", name);
    driver_ui_ptr->open_window(name);
  }

  void rendering_system::close_ui_window(const std::string_view name) {
    OTHER_ASSERT(driver_ui_ptr != nullptr, "Driver UI subsystem is not initialized. Cannot close UI window '{}'", name);
    driver_ui_ptr->close_window(name);
  }

  void rendering_system::close_all_windows() {
    OTHER_ASSERT(driver_ui_ptr != nullptr, "Driver UI subsystem is not initialized. Cannot close all UI windows");
    driver_ui_ptr->close_all_windows();
  }

  ImTextureID rendering_system::get_texture_id(const resource_handle& handle) const {
    OTHER_ASSERT(renderer_ptr != nullptr, "Renderer is not initialized in rendering system get_ui_texture_id.");
    return renderer_ptr->get_texture_id(handle);
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

  natural_t rendering_system::register_viewport(const std::string_view name, const viewport_definition& def) {
    natural_t id = FNV(name);
    if (std::ranges::find_if(viewports, [id](const viewport& vd) { return vd.id == id; }) != viewports.end()) {
      CORE_LOG_WARN("Viewport with name '{}' already exists.", name);
      return id;
    }

    auto* pl = renderer_ptr->get_pipeline(def.render_pipeline_name);
    if (pl == nullptr) {
      CORE_LOG_ERROR("Render pipeline '{}' specified for viewport '{}' does not exist.", def.render_pipeline_name, name);
      return 0;
    }

    opt<resource_handle> final_texture_opt = pl->get_final_output_texture();
    if (!final_texture_opt.has_value()) {
      CORE_LOG_ERROR("Pipeline {} must have a screen texture to be a valid viewport. Cannot register '{}'.", def.render_pipeline_name, name);
      return 0;
    }

    // make a unique texture for this viewport that we will blit to later to reuse the pipeline for other viewports
    resource_handle viewport_texture = get_renderer()->copy_texture(def.render_pipeline_name, *final_texture_opt, std::format("viewport-{}", name));
    viewports.push_back(viewport{
      .id = id,
      .name = std::string{ name },
      .pipeline = pl,
      .cam = def.cam,
      .texture = viewport_texture,
    });
    CORE_LOG_DEBUG("Registered viewport '{}' with pipeline '{}'", name, def.render_pipeline_name);
    return id;
  }

  void rendering_system::remove_viewport(natural_t vp_id) {
    auto it = std::ranges::find_if(viewports, [vp_id](const viewport& vp) { return vp.id == vp_id; });
    if (it != viewports.end()) {
      get_renderer()->destroy_texture(it->texture);
      viewports.erase(it);
      CORE_LOG_DEBUG("Removed viewport with ID '{}'", vp_id);
    } else {
      CORE_LOG_WARN("Tried to remove viewport with ID '{}', but no matching viewport was found.", vp_id);
    }
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

  void rendering_system::handle_viewport_resize_event(const value& data) {
    if (data.type() != value_type::VEC2) {
      CORE_LOG_ERROR("Invalid data type for viewport resize event. Expected VEC2.");
      return;
    }
    glm::vec2 viewport_size = data;
    get_driver().on_viewport_resize(viewport_size);
  }

  void rendering_system::register_builtin_resource_tags() {
    OTHER_ASSERT(renderer_ptr != nullptr, "Renderer is not initialized in register_builtin_resource_tags.");

    auto& reg = renderer_ptr->get_binding_registry();
    reg.register_per_frame(resource_tag(resource_tag::kCameraTag), &detail::upload_camera_buffer_per_frame);
    reg.register_per_frame(resource_tag(resource_tag::kLightTag), &detail::upload_light_buffer_per_frame);
    reg.register_per_frame(resource_tag(resource_tag::kSimulationEnvironmentTag), &detail::upload_simulation_environment_buffer_per_frame);
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
    reg.register_executor("window_sized_compute_dispatch", &detail::make_window_sized_compute_dispatch);
    reg.register_executor("voxelize", &detail::make_voxelize);
    reg.register_executor("generate_mipmaps", &detail::make_generate_mipmaps);
    reg.register_executor("downsample_chain", &detail::make_downsample_chain);
    reg.register_executor("debug_overlay", &detail::make_debug_overlay);
    reg.register_executor("debug_meshes", &detail::make_debug_meshes);
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

    /// load all of it's resources that are on disk as assets (shaders/textures/etc..)
    // for (auto& resource : pl->definition.textures) {
    //   kernel->get_core_system<asset_system>().add_texture_asset(resource.seed_texture_path.value_or(""));
    // }

    // if (debug_pipeline_names.empty() && add_debug_overlay) {
    //   add_debug_overlay_to_pipeline(itr->definition.name);
    // } else if (add_debug_overlay) {
    //   auto debug_it = std::ranges::find(debug_pipeline_names, itr->definition.name);
    //   if (add_debug_overlay && debug_it != debug_pipeline_names.end()) {
    //     add_debug_overlay_to_pipeline(itr->definition.name);
    //   }
    // }

    auto* pl_ptr = renderer_ptr->get_pipeline(itr->definition.name);
    get_driver().handle_rendering_pipeline_loaded(itr->asset_id, pl_ptr);

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

    get_driver().handle_rendering_pipeline_unloaded(itr->asset_id, renderer_ptr->get_pipeline(itr->definition.name));
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
      return [pass_name = def.name](pass_context& ctx) {
        ctx.draw_quad();
      };
    }

    render_graph::pass_executor make_compute_dispatch(const pipeline_pass_definition& def, render_pipeline* pl) {
      const auto& params = def.executor.params;
      auto groups_it = params.find("groups");
      OTHER_ASSERT(groups_it != params.end(), "compute_dispatch: pass '{}' missing 'groups' param", def.name);
      glm::vec3 groups = groups_it->second;

      shader::compute_barrier_type barrier = shader::compute_barrier_type::SHADER_IMAGE_ACCESS;
      if (auto b_it = params.find("barrier"); b_it != params.end()) {
        std::string bar_str = b_it->second;
        barrier = compute_barrier_type_from_string(bar_str);
      }
      return [g = groups, b = barrier, pass_name = def.name](pass_context& ctx) {
        ctx.dispatch(glm::uvec3(g), b);
      };
    };

    render_graph::pass_executor make_window_sized_compute_dispatch(const pipeline_pass_definition& def, render_pipeline* pl) {
      const auto& params = def.executor.params;
      auto groups_it = params.find("groups");
      OTHER_ASSERT(groups_it != params.end(), "window_sized_compute_dispatch: pass '{}' missing 'groups' param", def.name);
      glm::vec3 groups = groups_it->second;

      shader::compute_barrier_type barrier = shader::compute_barrier_type::SHADER_IMAGE_ACCESS;
      if (auto b_it = params.find("barrier"); b_it != params.end()) {
        std::string bar_str = b_it->second;
        barrier = compute_barrier_type_from_string(bar_str);
      }
      return [g = groups, b = barrier, pass_name = def.name, pl](pass_context& ctx) {
        OTHER_ASSERT(pl != nullptr, "window_sized_compute_dispatch: no render pipeline provided for pass '{}'", pass_name);
        const glm::ivec2 win = pl->get_window_size();
        const glm::uvec3 local = glm::uvec3(g.x, g.y, g.z);
        const glm::uvec3 groups((win.x + local.x - 1) / local.x, (win.y + local.y - 1) / local.y, 1u);
        ctx.dispatch(groups, b);
      };
    };

    render_graph::pass_executor make_voxelize(const pipeline_pass_definition& def, render_pipeline* pl) {
      return [params = def.executor.params, pl, pass_name = def.name](pass_context& ctx) {
        auto& api = ctx.get_renderer().rendering()->api();
        auto vol = pl->find_texture_by_name("voxel_texture");
        OTHER_ASSERT(vol.has_value(), "voxelize: 'voxel_texture' texture not found");

        auto voxel_dim = params.find("voxel_dim");
        OTHER_ASSERT(voxel_dim != params.end(), "voxelize: 'voxel_dim' parameter not found");
        OTHER_ASSERT(voxel_dim->second.type() == value_type::INT32, "voxelize: 'voxel_dim' parameter must be of type INT32");

        const int32_t res = static_cast<int32_t>(voxel_dim->second);
        api->set_viewport(0, 0, res, res);
        api->set_color_mask(false);
        api->set_depth_mask(false);
        api->set_depth_test(false);

        ctx.get_renderer().get_resource<texture>(*vol).bind_image(0, 0, true, 0, texture::format::RGBA16F, WRITE);
        ctx.draw_stream();

        api->set_color_mask(true);
        api->set_depth_mask(true);
        api->set_depth_test(true);

        shader::compute_barrier_type barrier_bits = (shader::compute_barrier_type)((uint8_t)shader::SHADER_IMAGE_ACCESS | (uint8_t)shader::TEXTURE_FETCH);
        api->memory_barrier(barrier_bits);
      };
    }

    render_graph::pass_executor make_generate_mipmaps(const pipeline_pass_definition& def, render_pipeline* pl) {
      OTHER_ASSERT(!def.outputs.empty(), "generate_mips: pass '{}' needs an output texture", def.name);
      opt<resource_handle> target = pl->find_texture_by_name(def.outputs.front().resource_name);
      OTHER_ASSERT(target.has_value(), "generate_mips: target not found for pass '{}'", def.name);
      return [target = *target](pass_context& ctx) {
        // ctx.get_renderer().rendering()->api()->generate_texture_mipmaps(target);  // thin backend wrapper
      };
    }

    render_graph::pass_executor make_downsample_chain(const pipeline_pass_definition& def, render_pipeline* pl) {
      OTHER_ASSERT(!def.outputs.empty(), "downsample_chain: pass '{}' must declare the pyramid as an output", def.name);
      const std::string target_name = def.outputs.front().resource_name;
      opt<resource_handle> target = pl->find_texture_by_name(target_name);
      OTHER_ASSERT(target.has_value(), "downsample_chain: target texture '{}' not found for pass '{}'", target_name, def.name);

      glm::ivec2 group_size = { 8, 8 };
      if (auto it = def.executor.params.find("groups"); it != def.executor.params.end()) {
        glm::vec3 g = it->second;
        group_size = { (int32_t)g.x, (int32_t)g.y };
      }

      return [target = *target, gs = group_size, pass_name = def.name](pass_context& ctx) {
        auto& r = ctx.get_renderer();
        auto& tex = r.get_resource<texture>(target);
        const uint32_t levels = tex.mip_levels;  // public field; getter optional
        OTHER_ASSERT(levels > 1, "downsample_chain: target of pass '{}' has <= 1 mip level", pass_name);

        glm::ivec2 sz = tex.get_size();  // base-level size
        const texture::format fmt = tex.get_format();
        for (uint32_t i = 0; i + 1 < levels; ++i) {
          const glm::ivec2 dst = { std::max(1, sz.x >> 1), std::max(1, sz.y >> 1) };
          tex.bind_image(0, i, true, 0, fmt, READ);       // src = i
          tex.bind_image(1, i + 1, true, 0, fmt, WRITE);  // dst = i+1
          // imageStore ignores viewport, shader self-bounds via imageSize(dst).
          ctx.dispatch({ (dst.x + gs.x - 1) / gs.x, (dst.y + gs.y - 1) / gs.y, 1u }, shader::compute_barrier_type::SHADER_IMAGE_ACCESS);
          sz = dst;
        }
        // if later pass samples pyramid, it needs to request TEXTURE_FETCH barrier. image-access alone doesn't order texture fetches.
      };
    }

    render_graph::pass_executor make_debug_overlay(const pipeline_pass_definition& def, render_pipeline* pl) {
      return [pass = def.name](pass_context& ctx) {
        ctx.draw_debug_vertices(builtin_debug_streams::kTris, mesh::TRIANGLES);
        ctx.draw_debug_vertices(builtin_debug_streams::kLines, mesh::LINES);
        ctx.draw_debug_vertices(builtin_debug_streams::kPoints, mesh::POINTS);
      };
    }

    render_graph::pass_executor make_debug_meshes(const pipeline_pass_definition& def, render_pipeline* pl) {
      return [pass = def.name](pass_context& ctx) {
        const render_stream& s = ctx.get_frame_data().debug_data;
        const size_t n = s.count(builtin_debug_streams::kMeshes);
        if (n == 0) {
          return;
        }

        const auto bytes = s.view(builtin_debug_streams::kMeshes);
        OTHER_ASSERT(bytes.size() == n * sizeof(debug_mesh_instance), "debug_meshes: stream byte size {} != {} instances", bytes.size(), n);

        const auto* inst = reinterpret_cast<const debug_mesh_instance*>(bytes.data());
        for (size_t i = 0; i < n; ++i) {
          ctx.draw_debug_mesh(inst[i]);
        }
      };
    }

    void upload_camera_buffer_per_frame(render_pipeline& r, const render_data& d, resource_handle h) {
      if (!d.primary_camera) {
        return;
      }

      auto gpu = d.primary_camera->to_gpu_data();
      r.upload_buffer(h, &gpu, sizeof(gpu));
    }

    void upload_light_buffer_per_frame(render_pipeline& r, const render_data& d, resource_handle h) {
      gpu::light_buffer buf{};
      for (size_t i = 0; i < d.lights.size() && i < gpu::kMaxLights; ++i) {
        buf.lights[i] = d.lights[i];
      }
      r.upload_to_handle(h, &buf, sizeof(gpu::light_buffer));
    }

    void upload_simulation_environment_buffer_per_frame(render_pipeline& r, const render_data& d, resource_handle h) {
      r.upload_to_handle(h, &d.simulation_environment, sizeof(gpu::simulation_environment_buffer));
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