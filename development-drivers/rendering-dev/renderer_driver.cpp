/**
 * \file renderer_driver.cpp
 **/
#include "renderer_driver.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_mouse.h>
#include <glad/glad.h>

#include "core/profiler.hpp"

#include "gpu_resource/renderer_resource.hpp"
#include "model/vertex.hpp"
#include "renderer/gpu_structs.hpp"
#include "renderer/render_graph.hpp"

#include "object/render_component.hpp"
#include "object/scene_object.hpp"

namespace other {
  namespace {

    real_t linear_to_gamma(real_t linear_component) {
      if (linear_component > 0) {
        return std::sqrt(linear_component);
      }

      return 0;
    }

    constexpr static const char* vert_shader_source = R"(
      #version 460 core

      layout (location = 0) in vec2 position;
      layout (location = 1) in vec2 tex_coords;

      out vec2 frag_tex_coords;

      void main() {
        gl_Position = vec4(position, 0.0, 1.0);
        frag_tex_coords = tex_coords;
      }
    )";

    constexpr static const char* frag_shader_source = R"(
      #version 460 core

      in vec2 frag_tex_coords;

      out vec4 frag_color;

      uniform sampler2D screen_texture;

      void main() {
        vec3 tex_color = texture(screen_texture, frag_tex_coords).rgb;
        frag_color = vec4(tex_color, 1.0);
      }
    )";

    constexpr static real_t quad_vertices[] = {
      -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
      -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
      1.0, 1.0f, 0.0f, 1.0f, 1.0f,
      1.0, -1.0f, 0.0f, 1.0f, 0.0f
    };

    constexpr float quad_vertices2[] = {
      -1.0f, 1.0f, 0.0f, 1.0f,
      -1.0f, -1.0f, 0.0f, 0.0f,
      1.0f, -1.0f, 1.0f, 0.0f,

      -1.0f, 1.0f, 0.0f, 1.0f,
      1.0f, -1.0f, 1.0f, 0.0f,
      1.0f, 1.0f, 1.0f, 1.0f
    };

    std::vector<vertex> get_cube_vertices();
    std::vector<index> get_cube_indices();

    std::pair<std::vector<vertex>, std::vector<index>> get_capsule_mesh(float radius, float height);

    constexpr static std::array lambertians = {
      gpu::lambertian{ glm::vec3(0.1f, 0.2f, 0.5f) },
      gpu::lambertian{ glm::vec3(0.8f, 0.8f, 0.f) },
    };

    constexpr static std::array metallics = {
      gpu::metal{ glm::vec3(0.8f, 0.8f, 0.8f), 0.3f },
      gpu::metal{ glm::vec3(0.8f, 0.6f, 0.2f), 1.f },
    };

    constexpr static std::array dielectrics = {
      gpu::dielectric{ glm::vec3(1.0f, 1.0f, 1.0f), 1.00 / 1.33 },
      gpu::dielectric{ glm::vec3(0.2f, 0.5f, 0.8f), 1.00 / 1.52 },
    };

    constexpr static std::array materials = {
      gpu::material{ gpu::MATERIAL_LAMBERTIAN, 0 },
      gpu::material{ gpu::MATERIAL_LAMBERTIAN, 1 },
      gpu::material{ gpu::MATERIAL_METAL, 0 },
      gpu::material{ gpu::MATERIAL_METAL, 1 },
      gpu::material{ gpu::MATERIAL_DIELECTRIC, 0 },
      gpu::material{ gpu::MATERIAL_DIELECTRIC, 1 },
    };

    constexpr static std::array spheres = {
      gpu::sphere{ glm::vec3(0.f, 0.f, -1.f), 0.5f },
      gpu::sphere{ glm::vec3(0.f, -100.5, -1.f), 100.f },
      gpu::sphere{ glm::vec3(1.f, 0.f, -1.f), 0.5f },
      gpu::sphere{ glm::vec3(-1.f, 0.f, -1.f), 0.5f },
    };

    constexpr static std::array objects = {
      gpu::object{ { gpu::SHAPE_SPHERE, 0 }, 0 },
      gpu::object{ { gpu::SHAPE_SPHERE, 1 }, 1 },
      gpu::object{ { gpu::SHAPE_SPHERE, 2 }, 3 },
      gpu::object{ { gpu::SHAPE_SPHERE, 3 }, 4 },
    };

  }  // namespace

  void renderer_driver::on_initialize() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    PROFILE_SECTION("renderer_driver::on_initialize");

    config_table config = configuration();
    toml::table& project_table = config.get_project_table();

    if (auto* gpu = project_table.at_path("run-on").as_string();
        /// if run-on flag doesn't exist, or is set to "gpu", then run on GPU
        gpu == nullptr || (gpu != nullptr && gpu->get() == "gpu")) {
      run_on_gpu = true;
    } else {
      run_on_gpu = false;
    }

    initialize_gpu();

    scene_object& capsule_obj = active_scene.create_object("Capsule", glm::vec3(2.5f, 0.f, 0.f));
    scene_object& cube_obj = active_scene.create_object("Cube", glm::vec3(0.f, 1.f, 0.f));

    scene_object& floor_obj = active_scene.create_object("Floor", glm::vec3(0.f, -1.5f, 0.f));
    scene_object& light_obj = active_scene.create_object("Light", glm::vec3(3.5f, 0.f, 0.f));
    scene_object& suzanne_obj = active_scene.create_object("Suzanne", glm::vec3(0.f, 0.f, 0.f));

    cube_id = cube_obj.id;
    capsule_id = capsule_obj.id;
    floor_id = floor_obj.id;
    light_id = light_obj.id;
    suzanne_id = suzanne_obj.id;

    // cube
    auto [cube_hash, cube_src] = model_source::load_model_source("Cube", get_cube_vertices(), get_cube_indices());
    /// capsule
    auto [cap_verts, cap_indices] = get_capsule_mesh(0.5f, 1.f);
    auto [capsule_hash, capsule_src] = model_source::load_model_source("Capsule", cap_verts, cap_indices);

    cube = cube_src->produce_model("Cube");
    capsule = capsule_src->produce_model("Capsule");

    auto image_size = renderer->get_window_size();

    SDL_WarpMouseInWindow(SDL_GetMouseFocus(), (float)image_size.x / 2, (float)image_size.y / 2);

    cam = serializer{}.read_from_file<camera>("artifacts/main_cam_data.bin");
    cam.look({ 0.f, 0.f, 3.f }, { 0.f, 0.f, 0.f });
    CORE_LOG_INFO("Camera data loaded from file: \n{}", type_data_handler<camera>::as_string("cam", cam));

    image_data.resize(image_size.x * image_size.y * kPixelStride);

    running = true;

    mouse.position = renderer->get_mouse_position();
    mouse.delta = glm::vec2(0.f, 0.f);

    shader& cube_sh = renderer->get_resource<shader>(cube_shader);
    shader& light_sh = renderer->get_resource<shader>(light_shader);

    render_component& cube_render = active_scene.add_component<render_component>(&cube_obj);
    cube_render.model = &cube;
    cube_render.shader_handle = &cube_sh;
    cube_render.material.diffuse_color = glm::vec3(0.8f, 0.2f, 0.2f);
    cube_render.material.diffuse_reflectivity = 0.5f;
    cube_render.material.specular_color = glm::vec3(0.8f, 0.8f, 0.8f);
    cube_render.material.specular_reflectivity = 0.5f;
    cube_render.material.emissivity = 0.1f;
    cube_render.material.shininess = 16.f;
    cube_render.material.transparency = 0.f;

    render_component& capsule_render = active_scene.add_component<render_component>(&capsule_obj);
    capsule_render.model = &capsule;
    capsule_render.shader_handle = &cube_sh;
    capsule_render.material.diffuse_color = glm::vec3(0.2f, 0.8f, 0.2f);
    capsule_render.material.diffuse_reflectivity = 0.5f;
    capsule_render.material.specular_color = glm::vec3(0.8f, 0.8f, 0.8f);
    capsule_render.material.specular_reflectivity = 0.5f;
    capsule_render.material.emissivity = 0.1f;
    capsule_render.material.shininess = 16.f;
    capsule_render.material.transparency = 0.f;

    transform& floor_transform = active_scene.get_transform(&floor_obj);
    render_component& floor_render = active_scene.add_component<render_component>(&floor_obj);
    floor_transform.local_scale = glm::vec3(10.f, 0.5f, 10.f);
    floor_render.model = &cube;
    floor_render.shader_handle = &cube_sh;
    floor_render.material.diffuse_color = glm::vec3(0.2f, 0.2f, 0.8f);
    floor_render.material.diffuse_reflectivity = 0.5f;
    floor_render.material.specular_color = glm::vec3(0.8f, 0.8f, 0.8f);
    floor_render.material.specular_reflectivity = 0.5f;
    floor_render.material.emissivity = 0.1f;
    floor_render.material.shininess = 16.f;
    floor_render.material.transparency = 0.f;

    transform& light_transform = active_scene.get_transform(&light_obj);
    render_component& light_render = active_scene.add_component<render_component>(&light_obj);
    gpu::point_light& light_plight = active_scene.add_component<gpu::point_light>(&light_obj);
    gpu::directional_light& light_dlight = active_scene.add_component<gpu::directional_light>(&light_obj);
    light_transform.local_scale = glm::vec3(0.1f, 0.1f, 0.1f);
    light_render.model = &cube;
    light_render.shader_handle = &light_sh;
    light_plight.light_position = light_transform.local_position;
    light_plight.color = glm::vec4(1.f, 1.f, 1.f, 1.f);
    light_dlight.direction = glm::vec3(0.f, -1.f, 0.f);
    light_dlight.color = glm::vec4(1.f, 1.f, 1.f, 1.f);

    auto [hash, suzanne_source] = model_source::load_model_source("resources/models/suzanne.fbx");  // ("resources/models/NewSponza_Curtains_FBX_YUp.fbx");
    OTHER_ASSERT(suzanne_source != nullptr, "Failed to load Suzanne model source.");

    suzanne = suzanne_source->produce_model("Suzanne");
    CORE_LOG_DEBUG("created model : {}", other::type_data_handler<model>::as_string("suzanne", suzanne));
    render_component& suzanne_render = active_scene.add_component<render_component>(&suzanne_obj);
    suzanne_render.model = &suzanne;
    suzanne_render.shader_handle = &cube_sh;
    suzanne_render.material.diffuse_color = glm::vec3(0.4f, 0.6f, 0.8f);
    suzanne_render.material.diffuse_reflectivity = 0.5f;
    suzanne_render.material.specular_color = glm::vec3(0.8f, 0.8f, 0.8f);
    suzanne_render.material.specular_reflectivity = 0.5f;
    suzanne_render.material.emissivity = 0.1f;
    suzanne_render.material.shininess = 16.f;
    suzanne_render.material.transparency = 0.f;

    size_t num_root_children = active_scene.get_object_count();
    CORE_LOG_INFO("Number of root children in the scene: {}", num_root_children);
  }

  namespace {

    void write_materials_to_buffer(gpu_buffer& buffer);
    void write_lambertian_to_buffer(gpu_buffer& buffer);
    void write_metal_to_buffer(gpu_buffer& buffer);
    void write_dielectrics_to_buffer(gpu_buffer& buffer);
    void write_spheres_to_buffer(gpu_buffer& buffer);
    void write_objects_to_buffer(gpu_buffer& buffer);

  }  // namespace

  void renderer_driver::run() {
    PROFILE_SECTION("renderer_driver::run");

    while (running) {
      MARK_NAMED_FRAME("Main Frame");

      PROFILE_SECTION("rendering-dev::main-loop");
      pump_events();

      if (!running) {
        break;
      }

      if (pressing_mouse_wheel) {
        PROFILE_SECTION("rendering-dev--update-camera");

        /// udpate camera data
        glm::vec2 mouse_pos = renderer->get_mouse_position();
        mouse.delta = mouse_pos - mouse.position;
        mouse.position = mouse_pos;

        glm::vec2 rel_pos;
        SDL_GetRelativeMouseState(&rel_pos.x, &rel_pos.y);

        cam.adjust_look_orientation(rel_pos.x, rel_pos.y);
      }

      {
        PROFILE_SECTION("rendering-dev--update-scene-buffers");

        gpu::camera_data cam_data = cam.to_gpu_data();
        renderer->get_resource<gpu_buffer>(camera_buffer_handle)
          .set_shader_resource(1, cube_shader)
          .set_data(&cam_data, sizeof(gpu::camera_data))
          .finalize_buffer();
        renderer->get_resource<gpu_buffer>(camera_buffer_handle)
          .set_shader_resource(1, light_shader)
          .set_data(&cam_data, sizeof(gpu::camera_data))
          .finalize_buffer();
      }

      scene_object& cube_obj = active_scene.get_object(cube_id);
      scene_object& capsule_obj = active_scene.get_object(capsule_id);
      scene_object& floor_obj = active_scene.get_object(floor_id);
      scene_object& suzanne_obj = active_scene.get_object(suzanne_id);
      scene_object& light_obj = active_scene.get_object(light_id);

      render_component* cube_render = active_scene.get_component<render_component>(&cube_obj);
      render_component* capsule_render = active_scene.get_component<render_component>(&capsule_obj);
      render_component* floor_render = active_scene.get_component<render_component>(&floor_obj);
      render_component* suzanne_render = active_scene.get_component<render_component>(&suzanne_obj);

      {
        PROFILE_SECTION("rendering-dev--update-light-and-material-buffers");

        gpu::point_light_buffer light_buffer_data;
        light_buffer_data.lights[0] = *active_scene.get_component<gpu::point_light>(&light_obj);

        // gpu::directional_light_buffer dir_light_buffer_data;
        // dir_light_buffer_data.lights[0] = *active_scene.get_component<gpu::directional_light>(&light_obj);

        gpu::graphics_material_buffer material_buffer_data;
        material_buffer_data.materials[0] = cube_render->material;
        material_buffer_data.materials[1] = capsule_render->material;
        material_buffer_data.materials[2] = floor_render->material;
        material_buffer_data.materials[3] = suzanne_render->material;

        renderer->get_resource<gpu_buffer>(point_light_buffer_handle)
          .set_shader_resource(2, cube_shader)
          .set_data(&light_buffer_data, sizeof(gpu::point_light_buffer))
          .finalize_buffer();

        // renderer->get_resource<gpu_buffer>(dir_light_buffer_handle)
        //   .set_shader_resource(3, cube_shader)
        //   .set_data(&dir_light_buffer_data, sizeof(gpu::directional_light_buffer))
        //   .finalize_buffer();

        renderer->get_resource<gpu_buffer>(material_buffer_handle)
          .set_shader_resource(4, cube_shader)
          .set_data(&material_buffer_data, sizeof(gpu::graphics_material_buffer))
          .finalize_buffer();
      }

      // render_graph graph;
      // graph
      //   /// start the render passes
      //   .start_pass(0, { window_size.x, window_size.y }, comp_shader_handle)
      //   .add_color_attachment(screen_texture_handle, 0)
      //   .end_pass()
      //   /// final pass (render to screen)
      //   .start_pass(1, { window_size.x, window_size.y }, screen_shader_handle)
      //   .add_color_attachment(screen_texture_handle, 0)
      //   .bind_execute_callback([](class renderer& r, void* user_data) {
      //     renderer_driver* driver = static_cast<renderer_driver*>(user_data);
      //     if (driver == nullptr) {
      //       CORE_LOG_ERROR("Renderer driver is null, cannot execute render pass.");
      //       return;
      //     }
      //     r.get_resource<mesh>(driver->quad_mesh_handle).draw();
      //   })
      //   .end_pass();

      /// renderer->render(graph);

#define USE_PIPELINE 1

      {
        PROFILE_SECTION("rendering-dev--render-frame");

#if USE_PIPELINE
        renderer->submit_model(cube_render->model, cube_render->shader_handle, cube_render->material, active_scene.get_world_transform(suzanne_id));
#endif

        renderer::frame_resources frame_resources{
          .model_buffer = model_buffer_handle,
          .material_buffer = material_buffer_handle,
        };
        renderer->begin_frame(&frame_resources);

        renderer->get_resource<framebuffer>(initial_pass).bind();
        renderer->get_resource<shader>(cube_shader)
          .bind()
          .set_uniform("num_point_lights", 1)
          .set_uniform("num_direction_lights", 0)
          .unbind();

#if USE_PIPELINE
        renderer->execute_draw_calls();
#else
        // renderer->get_resource<shader>(cube_shader)
        //   .set_uniform("model_matrix", active_scene.get_world_transform(&cube_obj))
        //   .set_uniform("material_index", 0);
        // renderer->get_resource<mesh>(cube.source->get_mesh_handle()).draw();
        // renderer->get_resource<shader>(cube_shader).unbind();

        // renderer->get_resource<shader>(cube_shader)
        //   .set_uniform("model_matrix", active_scene.get_world_transform(floor_id))
        //   .set_uniform("material_index", 1);
        // renderer->get_resource<mesh>(cube.source->get_mesh_handle()).draw();

        // renderer->get_resource<shader>(cube_shader)
        //   .set_uniform("model_matrix", active_scene.get_world_transform(capsule_id))
        //   .set_uniform("material_index", 2);
        // renderer->get_resource<mesh>(capsule.source->get_mesh_handle()).draw();

        // suzanne_render->model->draw(&s, 3, active_scene.get_world_transform(suzanne_id));
        renderer->get_resource<shader>(cube_shader)
          .bind()
          .set_uniform("model_matrix", active_scene.get_world_transform(suzanne_id))
          .set_uniform("material_index", 3);
        renderer->get_resource<mesh>(suzanne.source->get_mesh_handle()).draw();
        renderer->get_resource<shader>(cube_shader).unbind();

        renderer->get_resource<shader>(light_shader)
          .bind()
          .set_uniform("model_matrix", active_scene.get_world_transform(light_id));
        renderer->get_resource<mesh>(cube.source->get_mesh_handle()).draw();
        renderer->get_resource<shader>(light_shader).unbind();
#endif

        renderer->get_resource<framebuffer>(initial_pass).unbind();

        // renderer->get_resource<texture>(screen_texture_handle).bind(0);
        // renderer->get_resource<texture>(voxel_3d_texture_handle).bind(1);
        // /// voxelize scene into voxel 3d texture
        // renderer->get_resource<texture>(voxel_3d_texture_handle).unbind(1);
        // renderer->get_resource<texture>(screen_texture_handle).unbind(0);

        /// final pass (render to screen)
        renderer->get_resource<texture>(screen_texture_handle).bind(0);
        renderer->get_resource<shader>(screen_shader_handle).bind();
        renderer->get_resource<mesh>(quad_mesh_handle).draw();
        renderer->get_resource<shader>(screen_shader_handle).unbind();
        renderer->get_resource<texture>(screen_texture_handle).unbind(0);

        renderer->begin_ui_frame();
        // if (ImGui::Begin("Debug Controls")) {
        //   scene_object& light_obj = active_scene.get_object(light_id);
        //   gpu::directional_light* dir_light = active_scene.get_component<gpu::directional_light>(&light_obj);
        //   ImGui::DragFloat("Direction Light X", &dir_light->direction.x, 0.01f);
        //   ImGui::DragFloat("Direction Light Y", &dir_light->direction.y, 0.01f);
        //   ImGui::DragFloat("Direction Light Z", &dir_light->direction.z, 0.01f);
        // }
        // ImGui::End();
        renderer->end_ui_frame();
        renderer->end_frame();
      }
    }
  }

  void renderer_driver::on_shutdown() {
    PROFILE_SECTION("renderer_driver::on_shutdown");
    CORE_LOG_INFO("Shutting down terminal driver...");

    if (renderer != nullptr) {
      renderer->destroy_resource(material_buffer_handle);

      renderer->destroy_resource(camera_buffer_handle);
      renderer->destroy_resource(screen_texture_handle);

      renderer->destroy_resource(screen_shader_handle);
      renderer->destroy_resource(quad_mesh_handle);
    } else {
      /// cpu shutdown
    }

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
    switch (event->type) {
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        running = false;
        break;

      case SDL_EVENT_KEY_DOWN:
        if (SDLK_SPACE == event->key.key) {
          CORE_LOG_INFO("Camera data loaded from file: \n{}", type_data_handler<camera>::as_string("cam", cam));
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
      cam.position += cam.forward() * cam.sensitivity;
    }

    if ((flags & CAMERA_MOVE_BACKWARD) == CAMERA_MOVE_BACKWARD) {
      cam.position -= cam.forward() * cam.sensitivity;
    }

    if ((flags & CAMERA_MOVE_RIGHT) == CAMERA_MOVE_RIGHT) {
      cam.position += cam.right() * cam.sensitivity;
    }

    if ((flags & CAMERA_MOVE_LEFT) == CAMERA_MOVE_LEFT) {
      cam.position -= cam.right() * cam.sensitivity;
    }
  }

  void renderer_driver::initialize_gpu() {
    PROFILE_SECTION("renderer_driver::initialize_gpu");
    CORE_LOG_INFO("      ...on gpu");
    renderer = get_renderer();
    if (!renderer) {
      CORE_LOG_ERROR("Renderer backend is not initialized.");
      return;
    }
    renderer->set_clear_color(glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));

    quad_mesh_handle = renderer->create_resource("quad_mesh", resource_type::MESH);
    renderer->get_resource<mesh>(quad_mesh_handle)
      .set_primitive_type(mesh::primitive_type::TRIANGLES)
      .add_attribute("position", mesh::attribute_type::FLOAT, 2, 0)
      .add_attribute("tex_coords", mesh::attribute_type::FLOAT, 2, 2)
      .upload_vertex_buffer("quad_vertices", 6, quad_vertices2, sizeof(quad_vertices2))
      .finalize_mesh();

    auto image_size = renderer->get_window_size();
    screen_texture_handle = texture::create("screen_texture", texture::tex_type::TEXTURE_2D, texture::format::RGBA32F, image_size.x, image_size.y);

    voxel_3d_texture_handle = texture::create3d("voxel_3d_texture", texture::format::RGBA32F, { image_size.x, image_size.y, image_size.x }, true);

    initial_pass = renderer->create_resource("initial-pass-fb", resource_type::FRAMEBUFFER);
    renderer->get_resource<framebuffer>(initial_pass)
      .set_size(image_size.x, image_size.y)
      .set_clear_color({ 0.1f, 0.1f, 0.1f, 1.f })
      .add_attachment(screen_texture_handle, framebuffer::attachment_type::COLOR)
      .finalize_framebuffer();

    // comp_pass = renderer->create_resource("comp-pass-fb", resource_type::FRAMEBUFFER);
    // renderer->get_resource<framebuffer>(comp_pass)
    //   .set_size(image_size.x, image_size.y)
    //   .set_clear_color(glm::vec4(0.f, 0.f, 0.f, 1.f))
    //   .add_attachment("comp-pass", framebuffer::attachment_type::COLOR)
    //   .finalize_framebuffer();

    const auto settings = {
      shader::setting{ "MAX_LAMBERTIAN", std::to_string(gpu::kMaxLambertian) },
      shader::setting{ "MAX_METAL", std::to_string(gpu::kMaxMetal) },
      shader::setting{ "MAX_DIELECTRIC", std::to_string(gpu::kMaxDielectric) },

      shader::setting{ "MAX_SPHERES", std::to_string(gpu::kMaxSpheres) },
      shader::setting{ "MAX_OBJECTS", std::to_string(gpu::kMaxObjects) },

      shader::setting{ "USE_WEIGHT_COSINE_HEMISPHERE" },

      /// material indices
      shader::setting{ "MATERIAL_LAMBERTIAN", std::to_string(gpu::MATERIAL_LAMBERTIAN) },
      shader::setting{ "MATERIAL_METAL", std::to_string(gpu::MATERIAL_METAL) },
      shader::setting{ "MATERIAL_DIELECTRIC", std::to_string(gpu::MATERIAL_DIELECTRIC) },

      /// shape indices
      shader::setting{ "SPHERE_TYPE", std::to_string(gpu::SHAPE_SPHERE) },
    };
    screen_shader_handle = shader::create("screen_shader", vert_shader_source, frag_shader_source);
    cube_shader = shader::create("cube_shader", "resources/default.vert", "resources/cube.frag", {});
    light_shader = shader::create("light_shader", "resources/pure-white.vert", "resources/basic.frag", {});

    camera_buffer_handle = gpu_buffer::create("camera_buffer", gpu_buffer::buf_type::UNIFORM_BUFFER, gpu_buffer::usage::DYNAMIC);
    point_light_buffer_handle = gpu_buffer::create("point_light_buffer", gpu_buffer::buf_type::UNIFORM_BUFFER, gpu_buffer::usage::DYNAMIC);
    dir_light_buffer_handle = gpu_buffer::create("direction_light_buffer", gpu_buffer::buf_type::UNIFORM_BUFFER, gpu_buffer::usage::DYNAMIC);
    material_buffer_handle = gpu_buffer::create("material_buffer", gpu_buffer::buf_type::UNIFORM_BUFFER, gpu_buffer::usage::DYNAMIC);
    model_buffer_handle = gpu_buffer::create("model_matrix_buffer", gpu_buffer::buf_type::STORAGE_BUFFER, gpu_buffer::usage::DYNAMIC);

    SDL_HideCursor();
  }

  namespace {

    void write_materials_to_buffer(gpu_buffer& buffer) {
      gpu::material_buffer mat_buf;
      for (size_t i = 0; i < materials.size() && i < gpu::kMaxMaterials; ++i) {
        mat_buf.materials[i] = materials[i];
      }

      buffer.set_data(&mat_buf, sizeof(gpu::material_buffer))
        .finalize_buffer();
    }

    void write_lambertian_to_buffer(gpu_buffer& buffer) {
      gpu::lambertian_buffer lam_buf;
      for (size_t i = 0; i < lambertians.size() && i < gpu::kMaxLambertian; ++i) {
        lam_buf.materials[i] = lambertians[i];
      }

      buffer.set_data(&lam_buf, sizeof(gpu::lambertian_buffer))
        .finalize_buffer();
    }

    void write_metal_to_buffer(gpu_buffer& buffer) {
      gpu::metal_buffer metal_buf;
      for (size_t i = 0; i < metallics.size() && i < gpu::kMaxMetal; ++i) {
        metal_buf.materials[i] = metallics[i];
      }

      buffer.set_data(&metal_buf, sizeof(gpu::metal_buffer))
        .finalize_buffer();
    }

    void write_dielectrics_to_buffer(gpu_buffer& buffer) {
      gpu::dielectric_buffer dielectrics_buf;
      for (size_t i = 0; i < dielectrics.size() && i < gpu::kMaxDielectric; ++i) {
        dielectrics_buf.materials[i] = dielectrics[i];
      }

      buffer.set_data(&dielectrics_buf, sizeof(gpu::dielectric_buffer))
        .finalize_buffer();
    }

    void write_spheres_to_buffer(gpu_buffer& buffer) {
      gpu::sphere_buffer sphere_buf;
      for (size_t i = 0; i < spheres.size() && i < gpu::kMaxSpheres; ++i) {
        sphere_buf.spheres[i] = spheres[i];
      }

      buffer.set_data(&sphere_buf, sizeof(gpu::sphere_buffer))
        .finalize_buffer();
    }

    void write_objects_to_buffer(gpu_buffer& buffer) {
      gpu::object_buffer obj_buf;
      for (size_t i = 0; i < objects.size() && i < gpu::kMaxObjects; ++i) {
        obj_buf.objects[i] = objects[i];
      }

      buffer.set_data(&obj_buf, sizeof(gpu::object_buffer))
        .finalize_buffer();
    }

    std::vector<vertex> get_cube_vertices() {
      std::vector<vertex> vertices;
      vertices.resize(8);

      /* (-,-,+) */ vertices[0].position = { -1.f / 2.0f, -1.f / 2.0f, 1.f / 2.0f };
      /* (-,-,+) */ vertices[0].normal = { -1.0f, -1.0f, 1.0f };
      /* (-,-,+) */ vertices[0].tex_coord = { 0.f, 1.f };

      /* (+,-,+) */ vertices[1].position = { 1.f / 2.0f, -1.f / 2.0f, 1.f / 2.0f };
      /* (+,-,+) */ vertices[1].normal = { 1.0f, -1.0f, 1.0f };
      /* (+,-,+) */ vertices[1].tex_coord = { 1.f, 1.f };

      /* (+,+,+) */ vertices[2].position = { 1.f / 2.0f, 1.f / 2.0f, 1.f / 2.0f };
      /* (+,+,+) */ vertices[2].normal = { 1.0f, 1.0f, 1.0f };
      /* (+,+,+) */ vertices[2].tex_coord = { 1.f, 0.f };

      /* (-,+,+) */ vertices[3].position = { -1.f / 2.0f, 1.f / 2.0f, 1.f / 2.0f };
      /* (-,+,+) */ vertices[3].normal = { -1.0f, 1.0f, 1.0f };
      /* (-,+,+) */ vertices[3].tex_coord = { 0.f, 0.f };

      /* (-,-,-) */ vertices[4].position = { -1.f / 2.0f, -1.f / 2.0f, -1.f / 2.0f };
      /* (-,-,-) */ vertices[4].normal = { -1.0f, -1.0f, -1.0f };
      /* (-,-,-) */ vertices[4].tex_coord = { 0.f, 1.f };

      /* (+,-,-) */ vertices[5].position = { 1.f / 2.0f, -1.f / 2.0f, -1.f / 2.0f };
      /* (+,-,-) */ vertices[5].normal = { 1.0f, -1.0f, -1.0f };
      /* (+,-,-) */ vertices[5].tex_coord = { 1.f, 1.f };

      /* (+,+,-) */ vertices[6].position = { 1.f / 2.0f, 1.f / 2.0f, -1.f / 2.0f };
      /* (+,+,-) */ vertices[6].normal = { 1.0f, 1.0f, -1.0f };
      /* (+,+,-) */ vertices[6].tex_coord = { 1.f, 0.f };

      /* (-,+,-) */ vertices[7].position = { -1.f / 2.0f, 1.f / 2.0f, -1.f / 2.0f };
      /* (-,+,-) */ vertices[7].normal = { -1.0f, 1.0f, -1.0f };
      /* (-,+,-) */ vertices[7].tex_coord = { 0.f, 0.f };

      return vertices;
    }

    std::vector<index> get_cube_indices() {
      std::vector<index> indices;

      indices.resize(12);
      indices[0] = { 0, 1, 2 };
      indices[1] = { 2, 3, 0 };

      indices[2] = { 1, 5, 6 };
      indices[3] = { 6, 2, 1 };

      indices[4] = { 7, 6, 5 };
      indices[5] = { 5, 4, 7 };

      indices[6] = { 4, 0, 3 };
      indices[7] = { 3, 7, 4 };

      indices[8] = { 4, 5, 1 };
      indices[9] = { 1, 0, 4 };

      indices[10] = { 3, 2, 6 };
      indices[11] = { 6, 7, 3 };

      return indices;
    }

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