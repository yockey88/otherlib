/**
 * \file rendering-pipelines/default_instancing_pipeline.cpp
 **/
#include "rendering-pipelines/default_instancing_pipeline.hpp"

#include "renderer/camera.hpp"

namespace other {
  namespace {

    constexpr float quad_vertices2[] = {
      -1.0f, 1.0f, 0.0f, 1.0f,
      -1.0f, -1.0f, 0.0f, 0.0f,
      1.0f, -1.0f, 1.0f, 0.0f,

      -1.0f, 1.0f, 0.0f, 1.0f,
      1.0f, -1.0f, 1.0f, 0.0f,
      1.0f, 1.0f, 1.0f, 1.0f
    };

  }  // namespace

  void default_instancing_pipeline::prepare_frame(renderer::frame_resources* resources, render_data* data) {
    gpu::camera_data cam_data = data->primary_camera->to_gpu_data();
    upload_buffer("camera_buffer", &cam_data, sizeof(gpu::camera_data));

    gpu::point_light_buffer light_buffer_data;
    for (size_t i = 0; i < data->point_lights.size() && i < gpu::kMaxPointLights; ++i) {
      light_buffer_data.lights[i] = data->point_lights[i];
    }

    gpu::directional_light_buffer dir_light_buffer_data;
    for (size_t i = 0; i < data->directional_lights.size() && i < gpu::kMaxDirectionalLights; ++i) {
      dir_light_buffer_data.lights[i] = data->directional_lights[i];
    }

    upload_buffer("point_light_buffer", &light_buffer_data, sizeof(gpu::point_light_buffer));
    upload_buffer("direction_light_buffer", &dir_light_buffer_data, sizeof(gpu::directional_light_buffer));

    get_pass_shader("shading-pass")
      ->bind()
      .set_uniform("OE_num_point_lights", (int32_t)(data->point_lights.size() > gpu::kMaxPointLights ? gpu::kMaxPointLights : data->point_lights.size()))
      .set_uniform("OE_num_direction_lights", (int32_t)(data->directional_lights.size() > gpu::kMaxDirectionalLights ? gpu::kMaxDirectionalLights : data->directional_lights.size()))
      .set_uniform("OE_gbuff_albedo", 0)
      .set_uniform("OE_gbuff_normal", 1)
      .set_uniform("OE_gbuff_position", 2)
      .unbind();
  }

  void default_instancing_pipeline::create_resources() {
    const auto settings = { shader::setting{ "MAX_OBJECTS", std::to_string(gpu::kMaxObjects) } };

    geometry_pass_shader_handle = shader::create("geometry_pass_shader_handle", "resources/basic-instancing-gbuffer.vert", "resources/basic-instancing-gbuffer.frag", settings);
    shading_pass_shader_handle = shader::create("shading_pass_shader_handle", "resources/basic-shading.vert", "resources/basic-shading.frag", settings);
    screen_shader_handle = shader::create("screen_shader", "resources/basic-textured-quad.vert", "resources/basic-textured-quad.frag", settings);

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
    add_buffer_resource("point_light_buffer", gpu_buffer::buf_type::STORAGE_BUFFER, gpu_buffer::usage::DYNAMIC);
    add_buffer_resource("direction_light_buffer", gpu_buffer::buf_type::STORAGE_BUFFER, gpu_buffer::usage::DYNAMIC);
    add_buffer_resource("material_buffer", gpu_buffer::buf_type::STORAGE_BUFFER, gpu_buffer::usage::DYNAMIC);
    add_buffer_resource("model_buffer", gpu_buffer::buf_type::UNIFORM_BUFFER, gpu_buffer::usage::DYNAMIC);

    add_texture_resource("color_texture", window_size, framebuffer::attachment_type::COLOR, texture::tex_type::TEXTURE_2D, texture::format::RGBA32U);
    add_texture_resource("normal_texture", window_size, framebuffer::attachment_type::COLOR, texture::tex_type::TEXTURE_2D, texture::format::RGBA16F);
    add_texture_resource("position_texture", window_size, framebuffer::attachment_type::COLOR, texture::tex_type::TEXTURE_2D, texture::format::RGBA16F);
    add_texture_resource("screen_texture", window_size, framebuffer::attachment_type::COLOR);

    set_model_buffer("model_buffer");
    set_material_buffer("material_buffer");
    set_point_light_buffer("point_light_buffer");
    set_direction_light_buffer("direction_light_buffer");
    set_camera_buffer("camera_buffer");
  }

  void default_instancing_pipeline::build_render_passes() {
    auto window_size = get_renderer()->get_window_size();
    start_pass("geometry-pass", geometry_pass_shader_handle, render_pass::RENDER_PASS, window_size)
      .texture_resource("color_texture", framebuffer::COLOR, WRITE)
      .texture_resource("normal_texture", framebuffer::COLOR, WRITE)
      .texture_resource("position_texture", framebuffer::COLOR, WRITE)
      .buffer_resource("material_buffer", 0, READ)
      .buffer_resource("model_buffer", 1, READ)
      .buffer_resource("camera_buffer", 2, READ)
      .execution_callback([&](renderer& renderer, const render_graph::node* node, void* user_data) {
        renderer.execute_draw_calls();
      })
      .end_pass();

    start_pass("shading-pass", shading_pass_shader_handle, render_pass::RENDER_PASS, window_size)
      .clear_color(glm::vec4(0.2f, 0.2f, 0.2f, 1.f))
      .texture_resource("color_texture", framebuffer::COLOR, READ)
      .texture_resource("normal_texture", framebuffer::COLOR, READ)
      .texture_resource("position_texture", framebuffer::COLOR, READ)
      .texture_resource("screen_texture", framebuffer::COLOR, WRITE)
      .buffer_resource("direction_light_buffer", 0, READ)
      .buffer_resource("point_light_buffer", 1, READ)
      .buffer_resource("camera_buffer", 2, READ)
      .execution_callback([&](renderer& renderer, const render_graph::node* node, void* user_data) {
        renderer.get_resource<mesh>(quad_mesh_handle).draw();
      })
      .end_pass();

    start_pass("to-screen", screen_shader_handle, render_pass::RENDER_PASS, window_size, /* create_framebuffer = */ false)
      .texture_resource("screen_texture", framebuffer::COLOR, READ)
      .execution_callback([&](renderer& renderer, const render_graph::node* node, void* user_data) {
        get_pass_shader("to-screen")->set_uniform("OE_texture", 0);
        renderer.get_resource<mesh>(quad_mesh_handle).draw();
      })
      .end_pass();
  }

}  // namespace other