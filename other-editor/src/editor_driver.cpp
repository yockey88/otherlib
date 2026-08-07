/**
 * \file editor_driver.cpp
 **/
#include "editor_driver.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keycode.h>

#include "core/profiler.hpp"
#include "serialization/scene_serializer.hpp"
#include "thread/thread_safety.hpp"

#include "model/vertex.hpp"
#include "renderer/colors.hpp"

#include "object/camera_component.hpp"
#include "object/light_component.hpp"
#include "object/physics_component.hpp"
#include "object/render_component.hpp"
#include "object/scene_object.hpp"
#include "scene/scene.hpp"

#include "driver/systems/scene_system.hpp"
#include "ui/menu-bar/menu_item.hpp"
#include "ui/object-editor/object_editor.hpp"
#include "ui/project-creator/project_creator.hpp"
#include "ui/render-pipeline-ui/render_pipeline_editor.hpp"
#include "ui/render-pipeline-ui/render_pipeline_viewer.hpp"
#include "ui/scene-hierarchy/scene_hierarchy.hpp"
#include "ui/viewport/viewport.hpp"


namespace other {

  void editor_driver::on_early_initialize() {
    PROFILE_SECTION("editor_driver::on_early_initialize");
    auto& ui = get_ui();
    viewport_id = ui->register_window<ui::viewport>("viewport", context, *get_event_system(), get_renderer(), this);
    ui->register_window<ui::scene_hierarchy>("scene-hierarchy", context, *get_event_system(), this);
    ui->register_window<ui::object_editor>("object-editor", context, *get_event_system(), this);
    ui->register_window<ui::project_creator>("project-creator", context, *get_event_system());
    ui->register_window<ui::render_pipeline_viewer>("render-pipeline-viewer", context, *get_event_system());
    ui->register_window<ui::render_pipeline_editor>("render-pipeline-editor", context, *get_event_system());
  }

  void editor_driver::on_initialize() {
    PROFILE_SECTION("editor_driver::on_initialize");
    CORE_LOG_INFO("Initialized editor driver.");

    get_event_system()->register_event("viewport.clicked");
    get_event_system()->add_listener("viewport.clicked", [this](const value& data) {
      OTHER_ASSERT(data.type() == value_type::VEC2, "Expected viewport.clicked event data to be of type VEC2 representing the click position.");
      glm::vec2 click_position = data;
      CORE_LOG_INFO("Viewport clicked at position: ({}, {})", click_position.x, click_position.y);
    });

    auto* input_sys = subsystem<input_system>::get();
    OTHER_ASSERT(input_sys != nullptr, "Input system is null");
    input_sys->push_context("editor-controls");

    context.editor_camera.position = { 0.f, 1.f, 4.5f };
    context.editor_camera.sensitivity = 10.f;
    context.editor_camera.look_at({ 0.f, 0.f, 0.f });

    get_event_system()->add_listener("scene.activated", [this](const value& data) {
      OTHER_ASSERT(data.type() == value_type::UINT64, "Expected scene.activated event data to be of type UINT64 representing the active scene ID.");
      context.current_selection.scene_ptr = get_active_scene();
      OTHER_ASSERT(context.current_selection.scene_ptr != nullptr, "Active scene pointer is null in scene.activated event listener.");
      context.reset_scene_edit_tracking();
    });
    get_event_system()->add_listener("scene.deactivated", [this](const value& data) {
      OTHER_ASSERT(data.type() == value_type::UINT64, "Expected scene.deactivated event data to be of type UINT64 representing the active scene ID.");
      context.current_selection.scene_ptr = nullptr;
      context.reset_scene_edit_tracking();
    });

    /// editor.lua created the File menu before on_initialize ran; append the scene entry
    get_ui()->register_main_menu_bar_menu_item("File", ui::menu_item{
                                                         .name = "Save Scene",
                                                         .action = action{ std::function<void()>([this]() { save_active_scene(); }) },
                                                       });

    auto& r = get_renderer();
    const ostd::vector<vertex_attribute> vtx = {
      { value_type::VEC3, "OE_position", 0, sizeof(glm::vec3) },
      { value_type::VEC4, "OE_color", 1, sizeof(glm::vec4) },
    };
    r.register_draw_stream(builtin_debug_streams::kLines, { .element_size = sizeof(debug_vertex), .max_per_frame = 1u << 16, .draw_recipe = { "debug_overlay", mesh::LINES, vtx } });
    r.register_draw_stream(builtin_debug_streams::kTris, { .element_size = sizeof(debug_vertex), .max_per_frame = 1u << 16, .draw_recipe = { "debug_overlay", mesh::TRIANGLES, vtx } });
    r.register_draw_stream(builtin_debug_streams::kPoints, { .element_size = sizeof(debug_vertex), .max_per_frame = 1u << 14, .draw_recipe = { "debug_overlay", mesh::POINTS, vtx } });
    r.register_draw_stream(builtin_debug_streams::kMeshes, { .element_size = sizeof(debug_mesh_instance), .max_per_frame = 4096, .draw_recipe = {} });
  }

  void editor_driver::on_build_driver_input_map(input_map& map) {
    PROFILE_SECTION("editor_driver::on_build_driver_input_map");
    auto& ctx = map.add_context("editor-controls", true);
    ctx.add_action("move", action_value_type::AXIS_2D, false)
      // keyboard – each key contributes ±1 to one component
      .bind_key(key_code::W, modifier_flags::NONE, 1.f, 1)
      .bind_key(key_code::A, modifier_flags::NONE, 1.f, 0)
      .bind_key(key_code::S, modifier_flags::NONE, -1.f, 1)
      .bind_key(key_code::D, modifier_flags::NONE, -1.f, 0)
      // gamepad left stick
      .bind_gamepad_axis(gamepad_axis::LEFT_STICK_X, 0.5f, -1.f, 0)
      .bind_gamepad_axis(gamepad_axis::LEFT_STICK_Y, 0.5f, -1.f, 1);

    ctx.add_action("move_vertical", action_value_type::AXIS_1D)
      .bind_key(key_code::LEFT_SHIFT, modifier_flags::NONE, 1.f)
      .bind_key(key_code::LEFT_CTRL, modifier_flags::NONE, -1.f)
      .bind_gamepad_button(gamepad_button::RIGHT_BUMPER, 1.f)
      .bind_gamepad_button(gamepad_button::LEFT_BUMPER, -1.f);

    ctx.add_action("look", action_value_type::AXIS_2D)
      .bind_gamepad_axis(gamepad_axis::RIGHT_STICK_X, 0.5f, 1.f, 0)
      .bind_gamepad_axis(gamepad_axis::RIGHT_STICK_Y, 0.5f, 1.f, 1);

    ctx.add_action("orbit_hold")
      .bind_mouse_button(mouse_button::MIDDLE);

    ctx.add_action("undo")
      .bind_key(key_code::Z, modifier_flags::CTRL);
    ctx.add_action("redo")
      .bind_key(key_code::Y, modifier_flags::CTRL);
    ctx.add_action("save-scene")
      .bind_key(key_code::S, modifier_flags::CTRL);
  }

  void editor_driver::on_rendering_pipeline_loaded(natural_t asset_id, render_pipeline* pipeline) {
    OTHER_ASSERT(pipeline != nullptr, "Loaded rendering pipeline is null.");
    PROFILE_SECTION("editor_driver::on_rendering_pipeline_loaded");
    const auto& definition = pipeline->get_definition();
    if (definition.name == "default-instancing") {
      auto& renderer_ptr = get_renderer();

      auto def = get_debug_overlay_pipeline_definition();
      def.name = "editor-debug-rendering";
      renderer_ptr.add_post_processing_pipeline("default-instancing", def.name, def);
      auto* source_pipeline = renderer_ptr.get_pipeline("default-instancing");
      OTHER_ASSERT(source_pipeline != nullptr, "Source pipeline 'default-instancing' not found when setting up editor debug overlay.");

      opt<resource_handle> src_frame = source_pipeline->get_screen_texture();
      OTHER_ASSERT(src_frame.has_value(), "Source frame texture is not available in the default-instancing pipeline.");
      renderer_ptr.register_texture_resource(def.name, "frame", src_frame.value());

      viewport_id = context.register_viewport("editor-viewport", "editor-debug-rendering");
    }
  }

  void editor_driver::on_rendering_pipeline_unloaded(natural_t asset_id, render_pipeline* pipeline) {
    OTHER_ASSERT(pipeline != nullptr, "Unloaded rendering pipeline is null.");
    PROFILE_SECTION("editor_driver::on_rendering_pipeline_unloaded");
    const auto& definition = pipeline->get_definition();
    if (definition.name == "default-instancing") {
      get_renderer().remove_pipeline("editor-debug-rendering");
      context.remove_viewport(viewport_id);
    }
  }

  void editor_driver::on_begin_frame(render_data* data) {
    PROFILE_SECTION("editor_driver::on_begin_frame");
    if (data == nullptr) {
      return;
    }

    auto* scene = get_active_scene();
    if (scene == nullptr) {
      return;
    }

    auto draw = get_renderer().debug();

    if (scene->physics_debug_rendering()) {
      PROFILE_SECTION("editor_driver::on_begin_frame--physics_debug");
      scene->sync_edit_mode_physics_poses();  /// colliders track entity edits while not playing
      if (physics_world* world = scene->physics(); world != nullptr) {
        physics_api::physics_render_debug_data physics_debug = world->get_debug_render_data();
        for (size_t i = 0; i < physics_debug.debug_lines.size(); ++i) {
          draw.line(physics_debug.debug_lines[i].start, physics_debug.debug_lines[i].end, physics_debug.debug_line_colors[i]);
        }
        for (size_t i = 0; i < physics_debug.debug_triangles.size(); ++i) {
          const auto& tri = physics_debug.debug_triangles[i];
          glm::vec4 color = physics_debug.debug_triangle_colors[i];
          color.a = 0.25f;  /// any solid geometry jolt emits reads as a tint, not a wall
          draw.triangle(tri.v0, tri.v1, tri.v2, color);
        }
      }
    }

    glm::vec4 select_color = basic_colors::kGreen;
    if (context.has_selection()) {
      PROFILE_SECTION("editor_driver::on_begin_frame--selection_overlay");
      for (const auto& obj_id : context.current_selection.objects) {
        if (!scene->is_visible(obj_id)) {
          continue;
        }

        auto& obj = scene->get_object(obj_id);
        auto aabb = scene->get_bounding_box(obj.id);
        draw.aabb(aabb, select_color);

        auto world_trans = scene->get_world_transform(obj_id);

        glm::vec3 local_up = { 0, 1, 0 };
        glm::vec3 local_right = { 1, 0, 0 };
        glm::vec3 local_forward = { 0, 0, -1 };
        glm::vec3 world_up = glm::normalize(glm::vec3(world_trans * glm::vec4(local_up, 0)));
        glm::vec3 world_right = glm::normalize(glm::vec3(world_trans * glm::vec4(local_right, 0)));
        glm::vec3 world_forward = glm::normalize(glm::vec3(world_trans * glm::vec4(local_forward, 0)));
        draw.arrow(glm::vec3(0, 0, 0), world_up, basic_colors::kRed);
        draw.arrow(glm::vec3(0, 0, 0), world_right, basic_colors::kGreen);
        draw.arrow(glm::vec3(0, 0, 0), world_forward, basic_colors::kBlue);

        /// obj_model.source stays null until the model asset finishes its async load
        if (auto* render = scene->try_get_component<render_component>(obj.id);
            render != nullptr && render->obj_model.source != nullptr) {
          draw.mesh(render->obj_model.source->get_mesh_handle(), world_trans, select_color, true);
        }

        if (auto* pl_comp = scene->try_get_component<point_light_component>(obj.id);
            pl_comp != nullptr) {
          glm::vec4 world_light_pos = world_trans * glm::vec4(pl_comp->light.position, 1.f);
          draw.sphere(glm::vec3(world_light_pos), 0.2, pl_comp->light.color, 32);
        }

        if (auto* camera_comp = scene->try_get_component<camera_component>(obj.id);
            camera_comp != nullptr) {
          camera::clip_planes save = camera_comp->camera.clip;
          constexpr camera::clip_planes debug_clip{
            .near_plane = 0.33f,
            .far_plane = 15.f,
          };
          /// the render camera's pose comes from camera.position/direction (see
          ///  render_scene_to_viewports), not the object's transform, so inverse(view_proj)
          ///  alone is the NDC->world map; image_size holds the scene viewport's size, which
          ///  is the aspect the game camera actually renders with
          camera_comp->camera.clip = debug_clip;
          glm::mat4 view_proj = camera_comp->camera.get_projection_matrix(glm::ivec2(camera_comp->camera.image_size)) * camera_comp->camera.get_view_matrix();
          camera_comp->camera.clip = save;
          draw.frustum(glm::inverse(view_proj), basic_colors::kBlue);
        }
      }
    }
  }

  void editor_driver::on_shutdown() {
    PROFILE_SECTION("editor_driver::on_shutdown");
    context.remove_all_viewports();
  }

  void editor_driver::update_running() {
    PROFILE_SECTION("editor_driver::update_running");
    auto& kernel = get_kernel();
    auto& rendering_sys = kernel.get_core_system<rendering_system>();
    auto* s = get_active_scene();
    if (s != nullptr) {
      for (auto& vp : rendering_sys.get_viewports()) {
        OTHER_ASSERT(vp.pipeline != nullptr, "Viewport '{}' has null pipeline.", vp.name);
        if (vp.cam != nullptr) {
          continue;
        }

        vp.cam = s->get_primary_camera();
      }
    }

    context.tick_edit_tracker();
    update_input();
  }

  void editor_driver::on_scene_activated(natural_t scene_id) {
    PROFILE_SECTION("editor_driver::on_scene_activated");
    auto* s = get_kernel().get_core_system<scene_system>().get_scene(scene_id);
    OTHER_ASSERT(s != nullptr, "Activated scene with ID {} not found in scene system.", scene_id);

    context.scene_viewport_handle = context.register_viewport("scene-viewport", "default-instancing", s->get_primary_camera());
  }

  void editor_driver::on_scene_played(natural_t scene_id) {
    context.capture_playback_selection();
  }

  void editor_driver::on_scene_stopped(natural_t scene_id) {
    context.restore_playback_selection();
  }

  void editor_driver::on_scene_deactivated(natural_t scene_id) {
    context.remove_viewport(context.scene_viewport_handle);
  }

  void editor_driver::update_input() {
    PROFILE_SECTION("editor_driver::update_input");
    auto* input_sys = subsystem<input_system>::get();
    OTHER_ASSERT(input_sys != nullptr, "Input system is null");

    auto vp = context.get_viewport(viewport_id);
    if (vp.cam == nullptr || !vp.hovered) {
      return;
    }

    const bool is_looking_around = input_sys->is_action_pressed("orbit_hold");
    if (!is_looking_around) {
      SDL_SetWindowRelativeMouseMode(subsystem<renderer_backend>::get()->get_main_window(), false);
      return;
    }

    glm::vec2 move = input_sys->get_action_value_2d("move");
    glm::vec2 look = input_sys->get_action_value_2d("look");
    float vertical = input_sys->get_action_value("move_vertical");

    if (glm::length(move) > 0.01f || glm::abs(vertical) > 0.01f) {
      float speed = 0.1f;

      context.editor_camera.position += context.editor_camera.forward() * move.y * speed;
      context.editor_camera.position += context.editor_camera.right() * move.x * speed;
      context.editor_camera.position += context.editor_camera.up() * vertical * speed;
    }

    if (glm::length(look) > 0.01f) {
      context.editor_camera.adjust_look_orientation(look.x, look.y);
    } else if (is_looking_around) {
      SDL_SetWindowRelativeMouseMode(subsystem<renderer_backend>::get()->get_main_window(), true);
      glm::vec2 mouse_delta = input_sys->get_mouse_delta();
      context.editor_camera.adjust_look_orientation(mouse_delta.x * 0.1f, mouse_delta.y * 0.1f);
    } else {
      SDL_SetWindowRelativeMouseMode(subsystem<renderer_backend>::get()->get_main_window(), false);
    }
  }

  void editor_driver::on_create_project() {
    auto& ui = get_ui();
    ui->open_window("project-creator");
  }

  void editor_driver::on_input_event(const input_state_change_event& event) {
    if (!event.pressed) {
      return;
    }

    if (event.action_name == "undo") {
      context.undo_scene_edit();
    } else if (event.action_name == "redo") {
      context.redo_scene_edit();
    } else if (event.action_name == "save-scene") {
      save_active_scene();
    }
  }

  void editor_driver::save_active_scene() {
    PROFILE_SECTION("editor_driver::save_active_scene");
    scene* s = get_active_scene();
    if (s == nullptr) {
      CORE_LOG_WARN("No active scene to save.");
      return;
    }

    if (!s->source_path.has_value()) {
      CORE_LOG_ERROR("Scene '{}' has no scene document path to save to (in-memory scenes cannot be saved yet).", s->name);
      return;
    }

    const serialization::scene_document doc = serialization::capture_scene(*s, serialization::default_codec_services());
    if (serialization::save_scene_document(doc, *s->source_path)) {
      CORE_LOG_INFO("Saved scene '{}' to '{}' ({} objects).", s->name, s->source_path->string(), doc.objects.size());
    } else {
      CORE_LOG_ERROR("Failed to save scene '{}' to '{}'.", s->name, s->source_path->string());
    }
  }

  std::vector<selected_draw> editor_driver::get_selection_draws() const {
    PROFILE_SECTION("editor_driver::get_selection_draws");
    std::vector<selected_draw> draws;
    if (!context.has_selection()) {
      return draws;
    }

    auto* scene = context.current_selection.scene_ptr;
    if (scene == nullptr) {
      return draws;
    }

    for (const auto& obj_id : context.current_selection.objects) {
      /// selection ids can be stale for a frame around a snapshot restore — skip
      ///  entries that no longer resolve
      scene_object* selected = scene->find_object(obj_id);
      if (selected == nullptr) {
        continue;
      }
      auto& obj = *selected;
      auto* render = scene->try_get_component<render_component>(obj.id);
      auto aabb = scene->get_bounding_box(obj.id);
      if (aabb == bounding_box::empty) {
        aabb = bounding_box(glm::vec3(-0.5f), glm::vec3(0.5f));
      }

      /// obj_model.source stays null until the model asset finishes its async load —
      ///  draw the model outline only once it is ready
      if (render != nullptr && render->obj_model.source != nullptr) {
        const glm::mat4 world = scene->get_world_transform(obj_id);
        model& m = render->obj_model;
        const auto& submeshes = m.source->source_data().submeshes;

        for (uint32_t sm_idx : m.submesh_indices) {
          const submesh& sm = submeshes[sm_idx];
          draws.emplace_back() = selected_draw{
            .call = draw_call{
              .mesh_handle = m.source->get_mesh_handle(),
              .submesh_index = sm_idx,
              .instance_count = 1,  // submit_draw_call asserts > 0
              .vertex_offset = sm.base_vertex,
              .vertex_count = sm.vert_cnt,
              .index_offset = sm.base_idx,
              .index_count = sm.idx_cnt,
            },
            .key = mesh_key{
              .model_source_handle = m.source->get_mesh_handle(),
              .render_state = POLYGON_MODE_FILL,
              .draw_mode = mesh::TRIANGLES,
              .submesh_index = sm_idx,
            },
            .world = world,
          };
        }
      }
    }

    return draws;
  }

  pipeline_definition editor_driver::get_debug_overlay_pipeline_definition() const {
    PROFILE_SECTION("editor_driver::get_debug_overlay_pipeline_definition");
    pipeline_definition def;
    def.name = "debug-overlay";

    def.textures.push_back({ .name = "frame", .use_window_size = true, .format = texture::format::RGBA16F });
    def.textures.push_back({ .name = "depth", .use_window_size = true, .format = texture::format::DEPTHF });
    def.textures.push_back({ .name = "debug_frame", .use_window_size = true, .format = texture::format::RGBA16F });
    def.buffers.push_back({ .name = "camera_buffer", .type = gpu_buffer::UNIFORM_BUFFER, .usage = gpu_buffer::DYNAMIC, .tag = resource_tag(resource_tag::kCameraTag) });
    def.shaders.push_back({ .name = "blit", .vertex_path = "resources/basic-textured-quad.vert", .fragment_path = "resources/debug-overlay-blit.frag" });
    def.shaders.push_back({ .name = "debug_overlay", .vertex_path = "resources/debug-overlay.vert", .fragment_path = "resources/debug-overlay.frag" });
    def.shaders.push_back({ .name = "debug_meshes", .vertex_path = "resources/debug-mesh.vert", .fragment_path = "resources/debug-overlay.frag" });

    def.passes.emplace_back() = {
      .name = "overlay-blit",
      .pass_type = render_pass::RENDER_PASS,
      .shader_name = "blit",
      .inputs = {
        { .resource_name = "frame", .uniform_name = "OE_frame", .binding = 0 },
        { .resource_name = "depth", .uniform_name = "OE_depth", .binding = 1 },
      },
      .outputs = {
        { .resource_name = "debug_frame", .attachment = framebuffer::COLOR, .access = access_flags::WRITE },
      },
      .executor = { .name = "fullscreen_quad" },
    };
    def.passes.emplace_back() = {
      .name = "overlay-geometry",
      .shader_name = "debug_overlay",
      .inputs = {
        { .resource_name = "camera_buffer", .uniform_name = "OE_camera", .binding = 0 },
      },
      .outputs = {
        { .resource_name = "debug_frame", .attachment = framebuffer::COLOR, .access = access_flags::WRITE },
      },
      .executor = { .name = "debug_overlay" },
      .depends_on = { "overlay-blit" },
      .bindings = {
        { .name = "per_frame.camera_buffer", .tag = resource_tag(resource_tag::kCameraTag), .scope = binding_scope::PER_FRAME, .type = binding_type::UNIFORM_BUFFER, .binding = 0 },
      },
      .clear_flags = framebuffer::DEPTH_BIT,
      .override_fb_clear = true,
    };
    def.passes.emplace_back() = {
      .name = "overlay-meshes",
      .shader_name = "debug_meshes",
      .inputs = {
        { .resource_name = "camera_buffer", .uniform_name = "OE_camera", .binding = 0 },
      },
      .outputs = {
        { .resource_name = "debug_frame", .attachment = framebuffer::COLOR, .access = access_flags::WRITE },
      },
      .executor = { .name = "debug_meshes" },
      .depends_on = { "overlay-geometry" },
      .bindings = {
        { .name = "per_frame.camera_buffer", .tag = resource_tag(resource_tag::kCameraTag), .scope = binding_scope::PER_FRAME, .type = binding_type::UNIFORM_BUFFER, .binding = 0 },
      },
      .clear_flags = framebuffer::DEPTH_BIT,
      .override_fb_clear = true,
    };

    def.display_texture_name = "debug_frame";
    return def;
  }

}  // namespace other

OTHER_DRIVER(other::editor_driver);