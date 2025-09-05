/**
 * \file renderer_driver.cpp
 **/
#include "renderer_driver.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_mouse.h>
#include <glad/glad.h>

#include "core/profiler.hpp"

#include "model/vertex.hpp"
#include "object/render_component.hpp"
#include "object/scene_object.hpp"
#include "renderer/gpu_structs.hpp"
#include "renderer/render_graph.hpp"
#include "renderer/render_pipeline.hpp"

#include "rendering-pipelines/default_instancing_pipeline.hpp"

#include "glm/gtc/type_ptr.hpp"

namespace other {
  namespace {

    std::vector<vertex> get_cube_vertices();
    std::vector<index> get_cube_indices();

    std::pair<std::vector<vertex>, std::vector<index>> get_capsule_mesh(float radius, float height);

  }  // namespace

  void renderer_driver::on_initialize() {
    PROFILE_SECTION("renderer_driver::on_initialize");

    config_table config = configuration();

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

      auto [hash, suzanne_source] = model_source::load_model_source("resources/models/suzanne3.fbx");
      OTHER_ASSERT(suzanne_source != nullptr, "Failed to load Suzanne model source.");

      suzanne = suzanne_source->produce_model("Suzanne");
      CORE_LOG_DEBUG("created model : {}", other::type_data_handler<model>::as_string("suzanne", suzanne));
      render_component& suzanne_render = active_scene.add_component<render_component>(&suzanne_obj);
      suzanne_render.model = &suzanne;
      suzanne_render.material.diffuse_color = glm::vec3(0.4f, 0.6f, 0.8f);
      suzanne_render.material.diffuse_reflectivity = 0.5f;
      suzanne_render.material.specular_color = glm::vec3(0.8f, 0.8f, 0.8f);
      suzanne_render.material.specular_reflectivity = 0.5f;
      suzanne_render.material.emissivity = 0.1f;
      suzanne_render.material.shininess = 16.f;
      suzanne_render.material.transparency = 0.f;

      size_t num_root_children = active_scene.get_object_count();
      CORE_LOG_INFO("Number of children in the scene: {}", num_root_children);

      running = true;

      other_assembly = load_dotnet_module("build/other-csharp/Debug/OtherCs.dll");
    }
  }

  void renderer_driver::run() {
    PROFILE_SECTION("renderer_driver::run");

    while (running) {
      MARK_NAMED_FRAME("Main Frame");
      PROFILE_SECTION("rendering-dev::main-loop");

      pump_events();
      if (!running) {
        break;
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

      {
        PROFILE_SECTION("rendering-dev--render-frame");
        render_data scene_render_data = active_scene.prepare_render_data();

        renderer->begin_frame(&scene_render_data);
        renderer->render();

        renderer->begin_ui_frame();
        if (ImGui::Begin("Debug Window")) {
          if (ImGui::DragFloat3("Suzanne Color", glm::value_ptr(active_scene.get_component<render_component>(suzanne_id)->material.diffuse_color), 0.01f, 0.f, 1.0f)) {}
          if (ImGui::DragFloat3("Light Position", glm::value_ptr(active_scene.get_component<gpu::point_light>(light_id)->light_position), 0.1f)) {}
          if (ImGui::DragFloat3("Light Color", glm::value_ptr(active_scene.get_component<gpu::point_light>(light_id)->color), 0.01f, 0.f, 1.0f)) {}
        }
        ImGui::End();
        renderer->end_ui_frame();

        renderer->end_frame();
      }
    }
  }

  void renderer_driver::on_shutdown() {
    OTHER_ASSERT(renderer != nullptr, "Renderer is not initialized.");
    PROFILE_SECTION("renderer_driver::on_shutdown");
    CORE_LOG_INFO("Shutting down terminal driver...");

    unload_dotnet_module(other_assembly);

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

      case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (event->button.button == SDL_BUTTON_MIDDLE) {
          pressing_mouse_wheel = true;
        }
        break;

      case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event->button.button == SDL_BUTTON_MIDDLE) {
          pressing_mouse_wheel = false;
        }

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