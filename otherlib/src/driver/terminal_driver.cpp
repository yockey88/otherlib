/**
 * \file driver/terminal_driver.cpp
 **/
#include "driver/terminal_driver.hpp"

#include <SDL3/SDL_events.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>

#include "SDL3/SDL_keycode.h"


#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#include "core/arena_allocator.hpp"
#include "renderer/gpu_structs.hpp"

#include "math/constants.hpp"
#include "math/random.hpp"

namespace other {
  namespace {

    constexpr static const char* vert_shader_source = R"(
    #version 460 core

    layout (location = 0) in vec3 position;
    layout (location = 1) in vec2 tex_coords;

    out vec2 frag_tex_coords;

    void main() {
      gl_Position = vec4(position, 1.0);
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

    constexpr static float quad_vertices[] = {
      -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
      -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
      1.0, 1.0f, 0.0f, 1.0f, 1.0f,
      1.0, -1.0f, 0.0f, 1.0f, 0.0f
    };

    constexpr static std::array spheres = {
      gpu::sphere{
        glm::vec3(0.f, 0.f, 0.f),
        0.5f,
      },
      gpu::sphere{
        glm::vec3(0.f, -100.5, 0.f),
        100.f,
      },
    };

    constexpr static std::array materials = {
      gpu::material{
        glm::vec3(0.2f, 0.2f, 0.2f),
        0.15f,
      },
      gpu::material{
        glm::vec3(0.3f, 0.8f, 0.3f),
        0.15f,
      },
    };

    constexpr static std::array objects = {
      gpu::object{ { gpu::SHAPE_SPHERE, 0 }, 0 },
      gpu::object{ { gpu::SHAPE_SPHERE, 1 }, 1 },
    };

  }  // namespace

  glm::mat4& camera::get_view_matrix() {
    view_matrix = glm::lookAt(position, target, up);
    return view_matrix;
  }

  glm::mat4& camera::get_projection_matrix(const glm::ivec2& window_size) {
    projection_matrix = glm::perspective(glm::radians(fov), static_cast<float>(window_size.x) / window_size.y, clip.near_plane, clip.far_plane);
    return projection_matrix;
  }

  void terminal_driver::on_initialize() {
    CORE_LOG_DEBUG("Initializing terminal driver...");

    renderer = get_renderer();
    if (!renderer) {
      CORE_LOG_ERROR("Renderer backend is not initialized.");
      return;
    }
    renderer->set_clear_color(glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));

    quad_mesh_handle = renderer->create_resource("quad_mesh", resource_type::MESH);
    renderer->get_resource<mesh>(quad_mesh_handle)
      .set_primitive_type(mesh::primitive_type::TRIANGLE_STRIP)
      .add_attribute("position", mesh::attribute_type::FLOAT, 3, 0)
      .add_attribute("tex_coords", mesh::attribute_type::FLOAT, 2, 3)
      .upload_vertex_buffer("quad_vertices", 4, quad_vertices, sizeof(quad_vertices))
      .finalize_mesh();

    image_size = renderer->get_window_size();
    screen_texture_handle = texture::create("screen_texture", texture::tex_type::TEXTURE_2D, texture::format::RGBA32F, image_size.x, image_size.y, true);

    const auto settings = {
      shader::setting{ "DEBUG_PATCH" },
      shader::setting{ "MAX_MATERIALS", std::to_string(gpu::kMaxMaterials) },
      shader::setting{ "MAX_SPHERES", std::to_string(gpu::kMaxSpheres) },
      shader::setting{ "MAX_OBJECTS", std::to_string(gpu::kMaxObjects) },
      shader::setting{ "USE_WEIGHT_COSINE_HEMISPHERE" },
    };
    comp_shader_handle = shader::create("comp_shader", "resources/raytrace.comp", settings);
    screen_shader_handle = shader::create("screen_shader", vert_shader_source, frag_shader_source);

    camera_buffer_handle = gpu_buffer::create("camera_buffer", gpu_buffer::buf_type::UNIFORM_BUFFER, gpu_buffer::usage::DYNAMIC);
    scene_metadata_handle = gpu_buffer::create("scene_metadata", gpu_buffer::buf_type::UNIFORM_BUFFER, gpu_buffer::usage::DYNAMIC);
    ray_buffer_handle = gpu_buffer::create("ray_buffer", gpu_buffer::buf_type::UNIFORM_BUFFER, gpu_buffer::usage::DYNAMIC);

    material_buffer_handle = gpu_buffer::create("material_buffer", gpu_buffer::buf_type::UNIFORM_BUFFER, gpu_buffer::usage::DYNAMIC);
    sphere_buffer_handle = gpu_buffer::create("sphere_buffer", gpu_buffer::buf_type::UNIFORM_BUFFER, gpu_buffer::usage::DYNAMIC);
    object_buffer_handle = gpu_buffer::create("object_buffer", gpu_buffer::buf_type::UNIFORM_BUFFER, gpu_buffer::usage::DYNAMIC);

    cam.position = glm::vec3(0, 0, 1);
    cam.target = glm::vec3(0, 0, 0);
    cam.fov = 90.f;
    cam.sensitivity = 1.f;
    cam.constrain_pitch = true;

    pixel_sample_scale = 1.f / float(samples_per_pixel);

    float focal_length = 1.f;
    float viewport_height = 2.f;
    float viewport_width = viewport_height * (float(image_size.x) / image_size.y);

    glm::vec3 viewport_u = glm::vec3(viewport_width, 0.f, 0.f);
    glm::vec3 viewport_v = glm::vec3(0.f, viewport_height, 0.f);

    pixel_delta_u = viewport_u / float(image_size.x);
    pixel_delta_v = viewport_v / float(image_size.y);

    glm::vec3 viewport_upper_left = cam.position - glm::vec3(0, 0, focal_length) - viewport_u / 2.f - viewport_v / 2.f;
    pixel00_loc = viewport_upper_left + 0.5f * (pixel_delta_u + pixel_delta_v);

    image_data.resize(image_size.x * image_size.y * kPixelStride);

    running = true;

    mouse.position = renderer->get_mouse_position();
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

  }  // namespace

  void terminal_driver::run() {
    CORE_LOG_DEBUG("Running terminal driver...");
    while (running) {
      pump_events();

      if (!running) {
        break;
      }

      glm::ivec2 window_size = renderer->get_window_size();

      renderer->begin_frame();

      gpu::scene_metadata metadata;
      metadata.window_size = glm::vec4(window_size.x, window_size.y, 0, 0);

      metadata.object_data = {
        spheres.size(),
        objects.size(),
        materials.size(),
        0
      };

      metadata.samples_per_pixel = samples_per_pixel;
      metadata.max_depth = 50;

      renderer->get_resource<gpu_buffer>(scene_metadata_handle)
        .set_shader_resource(3, comp_shader_handle)
        .set_data(&metadata, sizeof(gpu::scene_metadata))
        .finalize_buffer();

      glm::mat4 view_mat = cam.get_view_matrix();
      glm::mat4 projection_mat = cam.get_projection_matrix(window_size);

      gpu::camera_data cam_data;
      cam_data.position = glm::vec4(cam.position, 1.f);
      cam_data.forward = glm::vec4(glm::normalize(cam.target - cam.position), 0.f);
      cam_data.camera_features = glm::vec4(cam.clip.near_plane, cam.clip.far_plane, 0.f, 0.f);
      cam_data.view_matrix = view_mat;
      cam_data.projection_matrix = projection_mat;

      renderer->get_resource<gpu_buffer>(camera_buffer_handle)
        .set_shader_resource(2, comp_shader_handle)
        .set_data(&cam_data, sizeof(gpu::camera_data))
        .finalize_buffer();

      gpu::ray_gen_data ray_data;
      ray_data.pixel00_loc = glm::vec4(pixel00_loc, 0.f);
      ray_data.pixel_delta_u = glm::vec4(pixel_delta_u, 0.f);
      ray_data.pixel_delta_v = glm::vec4(pixel_delta_v, 0.f);
      ray_data.square_sample = other::sample_square();

      renderer->get_resource<gpu_buffer>(ray_buffer_handle)
        .set_shader_resource(4, comp_shader_handle)
        .set_data(&ray_data, sizeof(gpu::ray_gen_data))
        .finalize_buffer();

      write_materials_to_buffer(renderer->get_resource<gpu_buffer>(material_buffer_handle).set_shader_resource(6, comp_shader_handle));
      write_spheres_to_buffer(renderer->get_resource<gpu_buffer>(sphere_buffer_handle).set_shader_resource(1, comp_shader_handle));
      write_objects_to_buffer(renderer->get_resource<gpu_buffer>(object_buffer_handle).set_shader_resource(5, comp_shader_handle));

      /// compute pass
      renderer->get_resource<texture>(screen_texture_handle).bind(0);
      renderer->get_resource<shader>(comp_shader_handle)
        .dispatch({ image_size.x, image_size.y, 1 }, shader::compute_barrier_type::SHADER_IMAGE_ACCESS);
      renderer->get_resource<texture>(screen_texture_handle).unbind(0);

      /// texture pass
      renderer->get_resource<texture>(screen_texture_handle).bind(0);
      renderer->get_resource<shader>(screen_shader_handle).bind();
      renderer->get_resource<mesh>(quad_mesh_handle).draw();
      renderer->get_resource<shader>(screen_shader_handle).unbind();
      renderer->get_resource<texture>(screen_texture_handle).unbind(0);

      renderer->end_frame();
    }
  }

  void terminal_driver::on_shutdown() {
    CORE_LOG_DEBUG("Shutting down terminal driver...");

    renderer->destroy_resource(sphere_buffer_handle);
    renderer->destroy_resource(object_buffer_handle);
    renderer->destroy_resource(material_buffer_handle);
    renderer->destroy_resource(camera_buffer_handle);
    renderer->destroy_resource(scene_metadata_handle);
    renderer->destroy_resource(screen_texture_handle);
    renderer->destroy_resource(comp_shader_handle);
    renderer->destroy_resource(screen_shader_handle);
    renderer->destroy_resource(quad_mesh_handle);

    renderer = nullptr;
  }

  void terminal_driver::on_event(SDL_Event* event) {
    enum move_flags : uint8_t {
      NONE = 0,
      MOVE_FORWARD = 1 << 0,
      MOVE_BACKWARD = 1 << 1,
      MOVE_LEFT = 1 << 2,
      MOVE_RIGHT = 1 << 3,
    };
    uint8_t flags = NONE;

    switch (event->type) {
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        running = false;
        break;

      case SDL_EVENT_KEY_DOWN:
        switch (event->key.key) {
          case SDLK_W:
            flags |= MOVE_FORWARD;
            break;
          case SDLK_S:
            flags |= MOVE_BACKWARD;
            break;
          case SDLK_A:
            flags |= MOVE_LEFT;
            break;
          case SDLK_D:
            flags |= MOVE_RIGHT;
            break;

          default:
            break;
        }
        break;
      default:
        break;
    }

    if (flags & MOVE_FORWARD) {
      cam.position -= cam.forward() * 0.1f;
    }
    if (flags & MOVE_BACKWARD) {
      cam.position += cam.forward() * 0.1f;
    }
    if (flags & MOVE_LEFT) {
      cam.position += glm::normalize(glm::cross(cam.forward(), cam.up)) * 0.1f;
    }
    if (flags & MOVE_RIGHT) {
      cam.position -= glm::normalize(glm::cross(cam.forward(), cam.up)) * 0.1f;
    }
  }

  ray terminal_driver::get_ray(int32_t i, int32_t j) const {
    glm::vec3 offset = sample_square();
    glm::vec3 pixel_sample = pixel00_loc +
      (float(i) + offset.x) * pixel_delta_u +
      (float(j) + offset.y) * pixel_delta_v;

    glm::vec3 origin = cam.position;
    glm::vec3 direction = pixel_sample - origin;

    return ray(origin, direction);
  }

  glm::vec3 terminal_driver::ray_color(const ray& r, uint32_t depth, const scene_object& obj) const {
    if (depth <= 0) {
      return glm::vec3(0.f, 0.f, 0.f);
    }

    intersection_info info;
    obj.intersect(r, { cam.clip.near_plane, infinity }, info);
    if (info.hit) {
      /// really want random in cone?
      glm::vec3 direction = info.normal + random_unit_vector();
      return reflectance * ray_color(ray(info.hit_point, direction), depth - 1, obj);
    }

    glm::vec3 unit_direction = glm::normalize(r.direction);
    float a = 0.5f * (unit_direction.y + 1.f);
    return (1.f - a) * glm::vec3(1.f, 1.f, 1.f) + a * glm::vec3(0.5f, 0.7f, 1.f);
  }

  void terminal_driver::write_pixel(const glm::ivec2& pixel, const glm::vec3& color) {
    size_t pixel_index = (pixel.y * image_size.x + pixel.x) * kPixelStride;

    static const interval intensity(0.000, 0.999);
    uint8_t rbyte = uint8_t(256 * intensity.clamp(linear_to_gamma(color.r)));
    uint8_t gbyte = uint8_t(256 * intensity.clamp(linear_to_gamma(color.g)));
    uint8_t bbyte = uint8_t(256 * intensity.clamp(linear_to_gamma(color.b)));

    image_data[pixel_index + 0] = rbyte;
    image_data[pixel_index + 1] = gbyte;
    image_data[pixel_index + 2] = bbyte;
    image_data[pixel_index + 3] = 255;
  }

  driver* create_terminal_driver(const config_table& config) {
    return arena_allocator<terminal_driver>{}.allocate(config);
  }

  void destroy_terminal_driver(driver* instance) {
    return arena_allocator<terminal_driver>{}.free(instance);
  }

}  // namespace other