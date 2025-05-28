/**
 * \file driver/terminal_driver.cpp
 **/
#include "driver/terminal_driver.hpp"

#include <random>

#include <SDL3/SDL_events.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>

#include "SDL3/SDL_keycode.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#include "core/arena_allocator.hpp"

namespace other {
  namespace {

    inline float rand_float() {
      static std::uniform_real_distribution<float> distribution(0.0, 1.0);
      static std::mt19937 generator;
      return distribution(generator);
    }

    inline float rand_float(float min, float max) {
      // Returns a random real in [min,max).
      return min + (max - min) * rand_float();
    }

    glm::vec3 sample_square() {
      /// Sample a point in the square [-0.5, 0.5] x [-0.5, 0.5]
      return glm::vec3(rand_float() - 0.5f, rand_float() - 0.5f, 0.f);
    }

    inline glm::vec3 random_vec3() {
      return glm::vec3(rand_float(), rand_float(), rand_float());
    }

    inline glm::vec3 random_vec3(float min, float max) {
      return glm::vec3(rand_float(min, max), rand_float(min, max), rand_float(min, max));
    }

    inline glm::vec3 random_unit_vector() {
      do {
        auto p = random_vec3(-1.f, 1.f);
        float lensq = glm::dot(p, p);
        if (epsilon < lensq && lensq <= 1.f) {
          return glm::normalize(p);
        }
      } while (true);
    }

    inline glm::vec3 random_in_hemisphere(const glm::vec3& normal) {
      glm::vec3 in_unit_sphere = random_unit_vector();
      if (glm::dot(in_unit_sphere, normal) > 0.f) {
        return in_unit_sphere;
      } else {
        return -in_unit_sphere;
      }
    }

    inline float linear_to_gamma(float linear_component) {
      if (linear_component > 0) {
        return std::sqrt(linear_component);
      }

      return 0;
    }

    __declspec(align(16)) struct sphere {
      glm::vec3 position;
      float radius;
      glm::vec3 albedo;
      float reflectance;
    };

    constexpr size_t kMaxSpheres = 100;
    __declspec(align(16)) struct sphere_buffer {
      sphere spheres[kMaxSpheres];
    };

    __declspec(align(16)) struct camera_data {
      glm::vec4 position;
      glm::vec4 forward;

      // near & far clip, padding x2
      glm::vec4 camera_features;

      glm::mat4 view_matrix;
      glm::mat4 projection_matrix;
    };

    __declspec(align(16)) struct scene_metadata {
      glm::vec4 window_size;
      int num_spheres;
      int samples_per_pixel = 100;
    };

    __declspec(align(16)) struct camera_buffer {
      glm::vec4 camera_position;
      glm::vec4 camera_forward;
    };

    __declspec(align(16)) struct ray_gen_data {
      glm::vec4 pixel00_loc;
      glm::vec4 pixel_delta_u;
      glm::vec4 pixel_delta_v;
      glm::vec3 square_sample;
    };

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

    static std::array<sphere, kMaxSpheres> spheres = {
      sphere{
        glm::vec3(0.f, 0.f, 0.f),
        0.5f,
        glm::vec3(0.2f, 0.2f, 0.2f),
        0.15f,
      },
      sphere{
        glm::vec3(0.f, -100.5, 0.f),
        100.f,
        glm::vec3(0.3f, 0.8f, 0.3f),
        0.15f,
      },
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
    CORE_LOG_DEBUG("Image size: [{}, {}]", image_size.x, image_size.y);
    screen_texture_handle = renderer->create_resource("screen_texture", resource_type::TEXTURE);
    renderer->get_resource<texture>(screen_texture_handle)
      .set_type(texture::tex_type::TEXTURE_2D)
      .set_format(texture::format::RGBA32F)
      .set_size(image_size.x, image_size.y)
      .set_filter(texture::filter::LINEAR, texture::filter::LINEAR)
      .set_wrap_mode(texture::wrap::CLAMP_TO_EDGE, texture::wrap::CLAMP_TO_EDGE)
      .finalize_image(0, true);

    std::string comp_shader_file = "";
    if (std::ifstream file("resources/raytrace.comp"); file.is_open()) {
      std::stringstream buffer;
      buffer << "#version 460 core\n";
      buffer << "#define MAX_SPHERES " << kMaxSpheres << "\n";
      buffer << file.rdbuf();
      file.close();
      comp_shader_file = buffer.str();
    } else {
      CORE_LOG_ERROR("Failed to open compute shader file.");
    }

    comp_shader_handle = shader::create("comp_shader", comp_shader_file, shader::source_type::COMPUTE_SHADER);
    screen_shader_handle = shader::create("screen_shader", vert_shader_source, frag_shader_source);

    sphere_buffer_handle = gpu_buffer::create("sphere_buffer", gpu_buffer::buf_type::UNIFORM_BUFFER, gpu_buffer::usage::DYNAMIC);
    camera_buffer_handle = gpu_buffer::create("camera_buffer", gpu_buffer::buf_type::UNIFORM_BUFFER, gpu_buffer::usage::DYNAMIC);
    scene_metadata_handle = gpu_buffer::create("scene_metadata", gpu_buffer::buf_type::UNIFORM_BUFFER, gpu_buffer::usage::DYNAMIC);
    ray_buffer_handle = gpu_buffer::create("ray_buffer", gpu_buffer::buf_type::UNIFORM_BUFFER, gpu_buffer::usage::DYNAMIC);

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

  void terminal_driver::run() {
    CORE_LOG_DEBUG("Running terminal driver...");
    while (running) {
      pump_events();

      if (!running) {
        break;
      }

      /// update mouse state and camera
      glm::ivec2 window_size = renderer->get_window_size();
      // glm::vec2 mouse_pos = renderer->get_mouse_position();
      // SDL_WarpMouseInWindow(SDL_GetMouseFocus(), float(window_size.x) / 2, float(window_size.y) / 2);

      float rel_x, rel_y;
      SDL_GetRelativeMouseState(&rel_x, &rel_y);
      glm::vec2 rel_pos = { rel_x, rel_y };
      cam.euler_angles.x += rel_pos.x * cam.sensitivity;
      cam.euler_angles.y -= rel_pos.y * cam.sensitivity;

      if (cam.constrain_pitch) {
        if (cam.euler_angles.y > 89.0f) {
          cam.euler_angles.y = 89.0f;
        }
        if (cam.euler_angles.y < -89.0f) {
          cam.euler_angles.y = -89.0f;
        }
      }

      glm::vec3 new_dir;
      new_dir.x = cos(glm::radians(cam.euler_angles.x)) * cos(glm::radians(cam.euler_angles.y));
      new_dir.y = sin(glm::radians(cam.euler_angles.y));
      new_dir.z = sin(glm::radians(cam.euler_angles.x)) * cos(glm::radians(cam.euler_angles.y));
      cam.target = cam.position + glm::normalize(new_dir);

      renderer->begin_frame();

      scene_metadata metadata;
      metadata.num_spheres = 2;
      metadata.window_size = glm::vec4(window_size.x, window_size.y, 0, 0);
      metadata.samples_per_pixel = 1;

      renderer->get_resource<gpu_buffer>(scene_metadata_handle)
        .set_shader_resource(3, comp_shader_handle)
        .set_data(&metadata, sizeof(scene_metadata))
        .finalize_buffer();

      glm::mat4 view_mat = cam.get_view_matrix();
      glm::mat4 projection_mat = cam.get_projection_matrix(window_size);

      camera_data cam_data;
      cam_data.position = glm::vec4(cam.position, 1.f);
      cam_data.forward = glm::vec4(glm::normalize(cam.target - cam.position), 0.f);
      cam_data.camera_features = glm::vec4(cam.clip.near_plane, cam.clip.far_plane, 0.f, 0.f);
      cam_data.view_matrix = view_mat;
      cam_data.projection_matrix = projection_mat;

      renderer->get_resource<gpu_buffer>(camera_buffer_handle)
        .set_shader_resource(2, comp_shader_handle)
        .set_data(&cam_data, sizeof(camera_data))
        .finalize_buffer();

      ray_gen_data ray_data;
      ray_data.pixel00_loc = glm::vec4(pixel00_loc, 0.f);
      ray_data.pixel_delta_u = glm::vec4(pixel_delta_u, 0.f);
      ray_data.pixel_delta_v = glm::vec4(pixel_delta_v, 0.f);
      ray_data.square_sample = sample_square();

      renderer->get_resource<gpu_buffer>(ray_buffer_handle)
        .set_shader_resource(4, comp_shader_handle)
        .set_data(&ray_data, sizeof(ray_gen_data))
        .finalize_buffer();

      sphere_buffer sphere_buf;
      for (size_t i = 0; i < metadata.num_spheres && i < kMaxSpheres; ++i) {
        sphere_buf.spheres[i] = spheres[i];
      }
      renderer->get_resource<gpu_buffer>(sphere_buffer_handle)
        .set_shader_resource(1, comp_shader_handle)
        .set_data(&sphere_buf, sizeof(sphere_buffer))
        .finalize_buffer();

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