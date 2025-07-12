/**
 * \file renderer/default_instancing_pipeline.cpp
 **/
#include "renderer/default_instancing_pipeline.hpp"

#include "renderer/camera.hpp"

namespace other {
  namespace {

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

    constexpr float quad_vertices2[] = {
      -1.0f, 1.0f, 0.0f, 1.0f,
      -1.0f, -1.0f, 0.0f, 0.0f,
      1.0f, -1.0f, 1.0f, 0.0f,

      -1.0f, 1.0f, 0.0f, 1.0f,
      1.0f, -1.0f, 1.0f, 0.0f,
      1.0f, 1.0f, 1.0f, 1.0f
    };

  }  // namespace

  void default_instancing_pipeline::create_resources() {
    const auto settings = { shader::setting{ "MAX_OBJECTS", std::to_string(gpu::kMaxObjects) } };
    instancing_shader = shader::create("instancing_shader", "resources/basic-instancing.vert", "resources/basic-instancing.frag", settings);
    screen_shader_handle = shader::create("screen_shader", vert_shader_source, frag_shader_source);

    quad_mesh_handle = get_renderer()->create_resource("quad_mesh", resource_type::MESH);
    get_renderer()
      ->get_resource<mesh>(quad_mesh_handle)
      .set_primitive_type(mesh::primitive_type::TRIANGLES)
      .add_attribute("position", mesh::attribute_type::FLOAT, 2, 0)
      .add_attribute("tex_coords", mesh::attribute_type::FLOAT, 2, 2)
      .upload_vertex_buffer("quad_vertices", 6, quad_vertices2, sizeof(quad_vertices2))
      .finalize_mesh();

    auto window_size = get_renderer()->get_window_size();
    add_buffer_resource("camera_buffer", gpu_buffer::buf_type::UNIFORM_BUFFER, gpu_buffer::usage::DYNAMIC);
    add_buffer_resource("point_light_buffer", gpu_buffer::buf_type::UNIFORM_BUFFER, gpu_buffer::usage::DYNAMIC);
    add_buffer_resource("direction_light_buffer", gpu_buffer::buf_type::UNIFORM_BUFFER, gpu_buffer::usage::DYNAMIC);
    add_buffer_resource("material_buffer", gpu_buffer::buf_type::UNIFORM_BUFFER, gpu_buffer::usage::DYNAMIC);
    add_buffer_resource("model_buffer", gpu_buffer::buf_type::UNIFORM_BUFFER, gpu_buffer::usage::DYNAMIC);
    add_texture_resource("screen_texture", window_size, framebuffer::attachment_type::COLOR);

    set_model_buffer("model_buffer");
    set_material_buffer("material_buffer");
  }

  void default_instancing_pipeline::build_render_passes() {
    auto window_size = get_renderer()->get_window_size();
    start_pass("geometry-pass", instancing_shader, render_pass::RENDER_PASS, window_size)
      .buffer_resource("material_buffer", READ)
      .buffer_resource("model_buffer", READ)
      .buffer_resource("camera_buffer", READ)
      .buffer_resource("point_light_buffer", READ)
      .buffer_resource("direction_light_buffer", READ)
      .texture_resource("screen_texture", framebuffer::COLOR, WRITE)
      .execution_callback([&](renderer& renderer, const render_graph::node* node, void* user_data) {
        renderer.execute_draw_calls();
      })
      .end_pass();

    start_pass("to-screen", screen_shader_handle, render_pass::RENDER_PASS, window_size, /* create_framebuffer = */ false)
      .texture_resource("screen_texture", framebuffer::COLOR, READ)
      .execution_callback([&](renderer& renderer, const render_graph::node* node, void* user_data) {
        renderer.get_resource<mesh>(quad_mesh_handle).draw();
      })
      .end_pass();
  }

  void default_instancing_pipeline::prepare_frame(render_data* data) {
    OTHER_ASSERT(data != nullptr, "Render data is null in default instancing pipeline.");
    OTHER_ASSERT(data->primary_camera != nullptr, "Primary camera is null in render data.");

    gpu::camera_data cam_data = data->primary_camera->to_gpu_data();
    get_resource<gpu_buffer>("camera_buffer")
      ->set_shader_resource(2, instancing_shader)
      .set_data(&cam_data, sizeof(gpu::camera_data))
      .finalize_buffer();

    gpu::point_light_buffer light_buffer_data;
    gpu::directional_light_buffer dir_light_buffer_data;
    for (size_t i = 0; i < data->point_lights.size() && i < gpu::kMaxPointLights; ++i) {
      light_buffer_data.lights[i] = data->point_lights[i];
    }
    for (size_t i = 0; i < data->directional_lights.size() && i < gpu::kMaxDirectionalLights; ++i) {
      dir_light_buffer_data.lights[i] = data->directional_lights[i];
    }

    get_resource<gpu_buffer>("point_light_buffer")
      ->set_data(&light_buffer_data, sizeof(gpu::point_light_buffer))
      .finalize_buffer();
    get_resource<gpu_buffer>("direction_light_buffer")
      ->set_data(&dir_light_buffer_data, sizeof(gpu::directional_light_buffer))
      .finalize_buffer();
    get_renderer()
      ->get_resource<shader>(instancing_shader)
      .bind()
      .set_uniform("OE_num_point_lights", 1)
      .set_uniform("OE_num_direction_lights", 1)
      .unbind();
  }

}  // namespace other