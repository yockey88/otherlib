/**
 * \file renderer_driver.cpp
 **/
#include "renderer_driver.hpp"

#include <chrono>
#include <cstdint>
#include <stack>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_mouse.h>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "core/profiler.hpp"

#include "model/animation.hpp"
#include "model/vertex.hpp"
#include "renderer/camera.hpp"
#include "renderer/gpu_structs.hpp"
#include "renderer/render_graph.hpp"
#include "renderer/render_pipeline.hpp"

#include "object/animation_controller.hpp"
#include "object/render_component.hpp"
#include "object/scene_object.hpp"

#include "rendering-pipelines/default_instancing_pipeline.hpp"
#include "scripting/execution_nodes/linear_algebra_nodes.hpp"
#include "scripting/execution_nodes/source_sink_nodes.hpp"
#include "ui/value_ui.hpp"

#define UI_ON 1

namespace other {
  namespace {

    std::vector<vertex> get_cube_vertices();
    std::vector<index> get_cube_indices();

    std::pair<std::vector<vertex>, std::vector<index>> get_capsule_mesh(float radius, float height);

  }  // namespace

  void renderer_driver::on_initialize(const command_line& cmd) {
    PROFILE_SECTION("renderer_driver::on_initialize");

    config_table config = configuration();

    io_context = std::make_unique<asio::io_context>();
    asset_mgr = make_scope<asset_handler>(*io_context);
    OTHER_ASSERT(asset_mgr != nullptr, "Failed to create asset handler in renderer_driver");

    {
      PROFILE_SECTION("renderer_driver::on_initialize--initialize-renderer");
      renderer = get_renderer();
      if (!renderer) {
        CORE_LOG_ERROR("Renderer backend is not initialized.");
        return;
      }
      renderer->set_clear_color(glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
      renderer->add_pipeline<default_instancing_pipeline>("Default Instancing Pipeline");
    }

    {
      PROFILE_SECTION("renderer_driver::on_initialize--scene-setup");
      scene_object& suzanne_obj = active_scene.create_object("Suzanne", glm::vec3(0.f, -0.5f, 0.f));
      scene_object& light_obj = active_scene.create_object("Light", glm::vec3(0.f, 2.f, 0.f));
      active_scene.add_object_tag(light_obj.id, "scene-ambient-light");

      scene_object& cam_obj = active_scene.create_object("Main Camera", glm::vec3(0.f, 0.f, 0.f));
      active_scene.add_object_tag(cam_obj.id, "main-camera");

      other::camera& cam = active_scene.add_component<camera>(&cam_obj);
      cam = serializer{}.read_from_file<camera>("artifacts/main_cam_data.bin");
      cam.sensitivity = 0.35f;
      cam.look({ 0.f, 1.f, 4.5f }, { 0.f, 0.f, 0.f });
      CORE_LOG_INFO("Camera data loaded from file: \n{}", type_data_handler<camera>::as_string("cam", cam));

      suzanne_id = suzanne_obj.id;
      light_id = light_obj.id;
      camera_id = cam_obj.id;

      mouse.position = renderer->get_mouse_position();
      mouse.delta = glm::vec2(0.f, 0.f);

      transform& light_transform = active_scene.get_transform(&light_obj);
      light_transform.local_scale = glm::vec3(0.1f, 0.1f, 0.1f);
      gpu::point_light& light_plight = active_scene.add_component<gpu::point_light>(&light_obj);
      light_plight.light_position = glm::vec3(0.f, 5.f, 0.f);
      light_plight.color = glm::vec3(1.f, 1.f, 1.f);
      gpu::directional_light& light_dlight = active_scene.add_component<gpu::directional_light>(&light_obj);
      light_dlight.direction = glm::vec3(0.f, -1.f, 0.f);
      light_dlight.color = glm::vec3(1.f, 1.f, 1.f);

      {
        PROFILE_SECTION("renderer_driver::on_initialize--load-assets");
        const auto& config = configuration();
        std::string model_path = config.get_value<std::string>("assets.test-model", "resources/models/suzanne3.fbx");
        suzanne_asset_id = asset_mgr->load_asset(model_path);
      }
    }

    events = make_scope<event_system>(net_context->io_context);
    OTHER_ASSERT(events != nullptr, "Failed to create event system in renderer_driver");

    exec_graph.add_node("Vec1", make_scope<vec3_source_node>(glm::vec3{ 1.0f, 2.0f, 3.0f }));
    exec_graph.add_node("Vec2", make_scope<vec3_source_node>(glm::vec3{ 4.0f, 5.0f, 6.0f }));
    exec_graph.add_node("AddVecs", make_scope<add_vec3_node>());
    exec_graph.connect_nodes("Vec1", 0, "AddVecs", 0);
    exec_graph.connect_nodes("Vec2", 0, "AddVecs", 1);

    node_editor = make_scope<ui::node_editor>(*events);
    for (const auto& n : exec_graph.nodes) {
      node_editor->add_editor_node(n.name, n.exec_node->get_num_inputs(), n.exec_node->get_num_outputs());
    }
    for (const auto& l : exec_graph.links) {
      const auto& from_node = exec_graph.get_node_by_id(l.from.node_id);
      const auto& to_node = exec_graph.get_node_by_id(l.to.node_id);
      node_editor->connect_node_pins(from_node.name, l.from.pin_index, to_node.name, l.to.pin_index);
    }

    test_value = uint32_t(69);

    CORE_LOG_INFO("Renderer driver initialized successfully.");
  }

  /// things we are debugging with the ui
  static glm::vec4 test_editor_bg_color = glm::vec4(0.03f, 0.03f, 0.03f, 0.7f);
  static float grid_step = 50.0f;
  static float major_grid_step = grid_step * 5.0f;
  static glm::vec4 grid_color = glm::vec4(0.2f, 0.2f, 0.2f, 0.4f);
  static glm::vec4 major_grid_color = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
  static float grid_thickness = 1.0f;
  static float major_thickness = 2.0f;

  void renderer_driver::run() {
    PROFILE_SECTION("renderer_driver::run");

    float delta_time = 0.0f;

    std::chrono::steady_clock::time_point last_time = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point current_time;

    running = true;
    while (running) {
      MARK_NAMED_FRAME("Main Frame");
      PROFILE_SECTION("rendering-dev::main-loop");

      current_time = std::chrono::steady_clock::now();
      delta_time = std::chrono::duration<float>(current_time - last_time).count();

      last_time = current_time;

      pump_events();
      if (!running) {
        break;
      }
      io_context->poll();
      asset_mgr->update_pipelines();

      if (!loaded_suzanne && asset_mgr->get_asset_state(suzanne_asset_id) == asset_state::LOADED) {
        uint64_t hash = asset_mgr->get_asset_hash(suzanne_asset_id);
        ref<model_source> suzanne_source = subsystem<renderer_backend>::get()->get_model_source(hash);
        if (suzanne_source != nullptr) {
          scene_object& suzanne_obj = active_scene.get_object(suzanne_id);
          suzanne_model = suzanne_source->produce_model("Suzanne");

          render_component& suzanne_render = active_scene.add_component<render_component>(&suzanne_obj);
          suzanne_render.animated = true;
          suzanne_render.model = &suzanne_model;
          suzanne_render.material.diffuse_color = glm::vec3(0.4f, 0.6f, 0.8f);
          suzanne_render.material.diffuse_reflectivity = 0.5f;
          suzanne_render.material.specular_color = glm::vec3(0.8f, 0.8f, 0.8f);
          suzanne_render.material.specular_reflectivity = 0.5f;
          suzanne_render.material.emissivity = 0.1f;
          suzanne_render.material.shininess = 16.f;
          suzanne_render.material.transparency = 0.f;

          if (const auto& animations = suzanne_source->get_animations(); !animations.empty()) {
            animation_controller& anim_ctrl = active_scene.add_component<animation_controller>(&suzanne_obj);
            anim_ctrl.anim_ptr = suzanne_source->get_animation(0);
            anim_ctrl.model_ptr = suzanne_render.model;

            for (auto& anim : animations) {
              CORE_LOG_DEBUG("Model Animation: Name: {}, Duration: {}, TicksPerSecond: {}, Channels: {}", anim.name, anim.duration, anim.ticks_per_second, anim.channels.size());

              for (auto& channel : anim.channels) {
                CORE_LOG_DEBUG("  Channel Node Name: {}, Position Keys: {}, Rotation Keys: {}, Scaling Keys: {}", channel.node_name, channel.position_keys.size(), channel.rotation_keys.size(), channel.scale_keys.size());
              }
            }
          }

          loaded_suzanne = true;
        }
      }

      SDL_SetWindowRelativeMouseMode(subsystem<renderer_backend>::get()->get_main_window(), pressing_mouse_wheel);

      if (pressing_mouse_wheel) {
        PROFILE_SECTION("rendering-dev--update-camera");

        /// udpate camera data
        glm::vec2 mouse_pos = renderer->get_mouse_position();
        mouse.delta = mouse_pos - mouse.position;
        mouse.position = mouse_pos;

        glm::vec2 rel_pos;
        SDL_GetRelativeMouseState(&rel_pos.x, &rel_pos.y);

        scene_object& cam_obj = active_scene.get_object(camera_id);
        camera* cam = active_scene.get_component<camera>(&cam_obj);
        cam->adjust_look_orientation(rel_pos.x, rel_pos.y);
      }

      /// update animation
      {
        PROFILE_SECTION("rendering-dev--updates");
        active_scene.update(delta_time);
        active_scene.late_update(delta_time);
      }

      {
        PROFILE_SECTION("rendering-dev--render-frame");
        render_data scene_render_data = active_scene.prepare_render_data();

        renderer->begin_frame(&scene_render_data);
        renderer->render();

        renderer->begin_ui_frame();

#if UI_ON

        auto* transform_comp = active_scene.get_component<transform>(suzanne_id);
        auto* render_comp = active_scene.get_component<render_component>(suzanne_id);
        if (render_comp != nullptr) {
          if (ImGui::Begin("Debug Window")) {
            if (ImGui::DragFloat3("Suzanne Position", glm::value_ptr(transform_comp->local_position), 0.1f)) {}
            if (ImGui::DragFloat3("Suzanne Color", glm::value_ptr(render_comp->material.diffuse_color), 0.01f, 0.f, 1.0f)) {}
            if (ImGui::DragFloat3("Light Position", glm::value_ptr(active_scene.get_component<gpu::point_light>(light_id)->light_position), 0.1f)) {}
            if (ImGui::DragFloat3("Light Color", glm::value_ptr(active_scene.get_component<gpu::point_light>(light_id)->color), 0.01f, 0.f, 1.0f)) {}

            ImGui::SeparatorText("Node Editor Debug/Configuration");
            ImGui::DragFloat4("Editor Background Color", glm::value_ptr(test_editor_bg_color), 0.01f, 0.f, 1.f);
            ImGui::DragFloat("Grid Step", &grid_step, 1.f, 10.f, 500.f);
            ImGui::DragFloat("Major Grid Step", &major_grid_step, 5.f, 50.f, 2500.f);
            ImGui::DragFloat4("Grid Color", glm::value_ptr(grid_color), 0.01f, 0.f, 1.f);
            ImGui::DragFloat4("Major Grid Color", glm::value_ptr(major_grid_color), 0.01f, 0.f, 1.f);
            ImGui::DragFloat("Grid Thickness", &grid_thickness, 0.1f, 0.1f, 10.f);
            ImGui::DragFloat("Major Grid Thickness", &major_thickness, 0.1f, 0.1f, 10.f);
          }
          ImGui::End();
        }

        node_editor->render();

        if (ImGui::Begin("Value Editor")) {
          ui::value_editor("Test Value", test_value);
        }
        ImGui::End();
#endif
        renderer->end_ui_frame();

        renderer->end_frame();
      }
    }
  }

  void renderer_driver::on_shutdown() {
    OTHER_ASSERT(renderer != nullptr, "Renderer is not initialized.");
    PROFILE_SECTION("renderer_driver::on_shutdown");
    CORE_LOG_INFO("Shutting down terminal driver...");

    asset_mgr->unload_asset(suzanne_asset_id);
    while (asset_mgr->get_num_pending_unloads() > 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    asset_mgr->purge_stores();
    asset_mgr = nullptr;

    renderer->remove_pipeline("Default Instancing Pipeline");
    renderer = nullptr;
  }

  void renderer_driver::on_event(SDL_Event* event) {
    PROFILE_SECTION("renderer_driver::on_event");
    enum camera_move_flags : uint8_t {
      NONE = 0,
      CAMERA_MOVE_FORWARD = 1 << 0,
      CAMERA_MOVE_BACKWARD = 1 << 1,
      CAMERA_MOVE_RIGHT = 1 << 2,
      CAMERA_MOVE_LEFT = 1 << 3,
      CAMERA_MOVE_UP = 1 << 4,
      CAMERA_MOVE_DOWN = 1 << 5
    };

    uint8_t flags = NONE;
    scene_object& cam_obj = active_scene.get_object(camera_id);
    camera* cam = active_scene.get_component<camera>(&cam_obj);

    switch (event->type) {
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        running = false;
        break;

      case SDL_EVENT_KEY_DOWN:
        if (SDLK_SPACE == event->key.key) {
          CORE_LOG_INFO("Camera state : \n{}", type_data_handler<camera>::as_string("cam", *cam));
        }
        if (SDLK_W == event->key.key) {
          flags |= CAMERA_MOVE_FORWARD;
        }
        if (SDLK_S == event->key.key) {
          flags |= CAMERA_MOVE_BACKWARD;
        }
        if (SDLK_A == event->key.key) {
          flags |= CAMERA_MOVE_LEFT;
        }
        if (SDLK_D == event->key.key) {
          flags |= CAMERA_MOVE_RIGHT;
        }
        break;

      case SDL_EVENT_MOUSE_WHEEL:
        /// zoom grid in/out
        if (event->wheel.y > 0) {
          grid_step += 1.0f;
          major_grid_step = grid_step * 5.0f;
        } else {
          grid_step = glm::max(1.0f, grid_step - 1.0f);
          major_grid_step = grid_step * 5.0f;
        }
        break;

      case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (event->button.button == SDL_BUTTON_MIDDLE) {
          pressing_mouse_wheel = true;
        }
        break;

      case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event->button.button == SDL_BUTTON_MIDDLE) {
          pressing_mouse_wheel = false;
        }
        break;

      default:
        break;
    }

    if (flags == NONE) {
      return;
    }

    if ((flags & CAMERA_MOVE_FORWARD) == CAMERA_MOVE_FORWARD) {
      cam->position += cam->forward() * cam->sensitivity;
    }

    if ((flags & CAMERA_MOVE_BACKWARD) == CAMERA_MOVE_BACKWARD) {
      cam->position -= cam->forward() * cam->sensitivity;
    }

    if ((flags & CAMERA_MOVE_RIGHT) == CAMERA_MOVE_RIGHT) {
      cam->position += cam->right() * cam->sensitivity;
    }

    if ((flags & CAMERA_MOVE_LEFT) == CAMERA_MOVE_LEFT) {
      cam->position -= cam->right() * cam->sensitivity;
    }
  }

  namespace {

    static void calc_ring(size_t segments, float radius, float y, float dy, float height, float actual_radius, std::vector<vertex>& vertices) {
      float seg_incr = 1.0f / (float)(segments - 1);
      for (size_t s = 0; s < segments; s++) {
        float x = glm::cos(float(M_PI * 2) * s * seg_incr) * radius;
        float z = glm::sin(float(M_PI * 2) * s * seg_incr) * radius;

        vertex& vertex = vertices.emplace_back();
        vertex.position = glm::vec3(actual_radius * x, actual_radius * y + height * dy, actual_radius * z);
        vertex.normal = glm::normalize(glm::vec3(x, y, z));
      }
    }

    std::pair<std::vector<vertex>, std::vector<index>> get_capsule_mesh(float radius, float height) {
      constexpr size_t subdivision_height = 8;
      constexpr size_t rings_body = subdivision_height + 1;
      constexpr size_t rings_total = subdivision_height + rings_body;
      constexpr size_t num_segments = 12;
      // needed to ensure that the wireframe is always visible
      constexpr float radius_modifier = 0.021f;

      std::vector<vertex> vertices;
      std::vector<index> indices;

      vertices.reserve(num_segments * rings_total);
      indices.reserve((num_segments - 1) * (rings_total - 1) * 2);

      float body_incr = 1.0f / (float)(rings_body - 1);
      float ring_incr = 1.0f / (float)(subdivision_height - 1);

      for (int r = 0; r < subdivision_height / 2; r++)
        calc_ring(num_segments, glm::sin(float(M_PI) * r * ring_incr), glm::sin(float(M_PI) * (r * ring_incr - 0.5f)), -0.5f, height, radius + radius_modifier, vertices);

      for (int r = 0; r < rings_body; r++)
        calc_ring(num_segments, 1.0f, 0.0f, r * body_incr - 0.5f, height, radius + radius_modifier, vertices);

      for (int r = subdivision_height / 2; r < subdivision_height; r++)
        calc_ring(num_segments, glm::sin(float(M_PI) * r * ring_incr), glm::sin(float(M_PI) * (r * ring_incr - 0.5f)), 0.5f, height, radius + radius_modifier, vertices);

      for (int r = 0; r < rings_total - 1; r++) {
        for (int s = 0; s < num_segments - 1; s++) {
          index& index1 = indices.emplace_back();
          index1.v0 = (uint32_t)(r * num_segments + s + 1);
          index1.v1 = (uint32_t)(r * num_segments + s + 0);
          index1.v2 = (uint32_t)((r + 1) * num_segments + s + 1);

          index& index2 = indices.emplace_back();
          index2.v0 = (uint32_t)((r + 1) * num_segments + s + 0);
          index2.v1 = (uint32_t)((r + 1) * num_segments + s + 1);
          index2.v2 = (uint32_t)(r * num_segments + s);
        }
      }

      return { vertices, indices };
    }

  }  // namespace

}  // namespace other