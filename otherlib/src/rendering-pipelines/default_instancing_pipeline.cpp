/**
 * \file rendering-pipelines/default_instancing_pipeline.cpp
 **/
#include "rendering-pipelines/default_instancing_pipeline.hpp"

#include <minwindef.h>

// #include "core/formatting.hpp"
// #include "math/orthonormal_basis.hpp"
// #include "serialization/reflection.hpp"

#include "model/model.hpp"
#include "renderer/camera.hpp"
#include "renderer/gpu_structs.hpp"

#include "glm/fwd.hpp"

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

    std::vector<vertex> get_cube_vertices();
    std::vector<index> get_cube_indices();

  }  // namespace

  void default_instancing_pipeline::on_prepare_frame(renderer::frame_resources* resources, render_data* data) {
    if (data == nullptr) {
      return;
    }

    gpu::camera_data cam_data = data->primary_camera->to_gpu_data();
    upload_buffer("camera_buffer", &cam_data, sizeof(gpu::camera_data));

    gpu::point_light_buffer light_buffer_data;
    for (size_t i = 0; i < data->point_lights.size() && i < gpu::kMaxPointLights; ++i) {
      light_buffer_data.lights[i] = data->point_lights[i];
    }

    glm::mat4 light_space_matrix = glm::mat4(1.0f);
    glm::vec3 light_pos = glm::vec3(1.f, 4.f, 1.f);

    gpu::directional_light_buffer dir_light_buffer_data;
    if (data->scene_ambient_light != nullptr) {
      dir_light_buffer_data.lights[0] = *data->scene_ambient_light;

      float near_plane = 1.0f, far_plane = 10.f;
      glm::mat4 light_projection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);

      /// tiny shift to avoid nans
      glm::vec3 light_target = glm::vec3(0.0f, 0.0f, 0.0f);
      glm::mat4 light_view = glm::lookAt(light_pos, light_target, glm::vec3(0.f, 1.f, 0.f));

      light_space_matrix = light_projection * light_view;
      get_pass_shader("shadow-map-pass")
        ->bind()
        .set_uniform("OE_light_space_matrix", light_space_matrix)
        .unbind();
    }

    upload_buffer("point_light_buffer", &light_buffer_data, sizeof(gpu::point_light_buffer));
    upload_buffer("direction_light_buffer", &dir_light_buffer_data, sizeof(gpu::directional_light_buffer));

    get_pass_shader("shading-pass")
      ->bind()
      .set_uniform("OE_light_space_matrix", light_space_matrix)
      .set_uniform("OE_light_position", light_pos)
      .set_uniform("OE_num_point_lights", (int32_t)(data->point_lights.size() > gpu::kMaxPointLights ? gpu::kMaxPointLights : data->point_lights.size()))
      .set_uniform("OE_num_direction_lights", (int32_t)(data->scene_ambient_light ? 1 : 0))
      .set_uniform("OE_gbuff_albedo", 0)
      .set_uniform("OE_gbuff_normal", 1)
      .set_uniform("OE_gbuff_position", 2)
      .set_uniform("OE_shadow_map", 3)
      .unbind();

    get_pass_shader("to-screen")
      ->bind()
      .set_uniform("OE_exposure", 1.0f)
      .unbind();
  }

#define POINT_LIGHT_SHADOW_MAPS 0
#define EXTRA_DEBUG_POST_PROCESSING 0

  void default_instancing_pipeline::create_resources() {
    const auto settings = {
      shader::setting{ "MAX_OBJECTS", std::to_string(gpu::kMaxObjects) },
      // Support up to 4 bone influences per vertex (common convention)
      shader::setting{ "MAX_VERTEX_BONE_INFLUENCE", "4" },
      shader::setting{ "MAX_BONES", std::to_string(gpu::kMaxObjects) },
    };

    geometry_pass_shader_handle = shader::create("geometry_pass_shader_handle", "resources/basic-instancing-gbuffer.vert", "resources/basic-instancing-gbuffer.frag", settings);
    shadow_map_pass_shader_handle = shader::create("shadow_map_pass_shader_handle", "resources/basic-instancing-shadow-map.vert", "resources/basic-instancing-shadow-map.frag", settings);
    point_light_shadow_pass_shader_handle = shader::create("point_light_shadow_pass_shader_handle", "resources/basic-instancing-point-light-shadow-map.vert", "resources/basic-instancing-point-light-shadow-map.geom", "resources/basic-instancing-point-light-shadow-map.frag", settings);
    shading_pass_shader_handle = shader::create("shading_pass_shader_handle", "resources/basic-shading.vert", "resources/basic-shading.frag", settings);
    // debug_processing_shader_handle = shader::create("debug_processing_shader_handle", "resources/debug-processing.vert", "resources/debug-processing.frag", settings);
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
    add_buffer_resource("model_buffer", gpu_buffer::buf_type::UNIFORM_BUFFER, gpu_buffer::usage::DYNAMIC);
    add_buffer_resource("bone_buffer", gpu_buffer::buf_type::UNIFORM_BUFFER, gpu_buffer::usage::DYNAMIC);
    add_buffer_resource("point_light_buffer", gpu_buffer::buf_type::STORAGE_BUFFER, gpu_buffer::usage::DYNAMIC);
    add_buffer_resource("direction_light_buffer", gpu_buffer::buf_type::STORAGE_BUFFER, gpu_buffer::usage::DYNAMIC);
    add_buffer_resource("light_matrix_buffer", gpu_buffer::buf_type::STORAGE_BUFFER, gpu_buffer::usage::DYNAMIC);
    add_buffer_resource("material_buffer", gpu_buffer::buf_type::STORAGE_BUFFER, gpu_buffer::usage::DYNAMIC);

    add_texture_resource("color_texture", window_size, texture::tex_type::TEXTURE_2D, texture::format::RGBA32U);
    add_texture_resource("normal_texture", window_size, texture::tex_type::TEXTURE_2D, texture::format::RGBA16F);
    add_texture_resource("position_texture", window_size, texture::tex_type::TEXTURE_2D, texture::format::RGBA16F);

    add_texture_resource("ambient_shadow_map", window_size, texture::tex_type::TEXTURE_2D, texture::format::DEPTHF);
    add_texture_resource("pl_shadow_map", window_size, texture::tex_type::TEXTURE_CUBE, texture::format::DEPTHF);

    add_texture_resource("debug_processed_texture", window_size, texture::tex_type::TEXTURE_2D, texture::format::RGBA16F);

    add_texture_resource("screen_texture", window_size, texture::tex_type::TEXTURE_2D, texture::format::RGBA16F);

    set_model_buffer("model_buffer");
    set_material_buffer("material_buffer");
    set_bone_buffer("bone_buffer");
    set_point_light_buffer("point_light_buffer");
    set_direction_light_buffer("direction_light_buffer");
    set_camera_buffer("camera_buffer");
  }

  void default_instancing_pipeline::build_render_passes() {
    auto window_size = get_renderer()->get_window_size();
    CORE_LOG_DEBUG("Building default instancing pipeline render passes with window size: {}x{}", window_size.x, window_size.y);

    start_pass("geometry-pass", geometry_pass_shader_handle, render_pass::RENDER_PASS, window_size)
      .texture_resource("color_texture", framebuffer::COLOR, WRITE)
      .texture_resource("normal_texture", framebuffer::COLOR, WRITE)
      .texture_resource("position_texture", framebuffer::COLOR, WRITE)
      .buffer_resource("material_buffer", 0, READ)
      .buffer_resource("model_buffer", 1, READ)
      .buffer_resource("camera_buffer", 2, READ)
      .buffer_resource("bone_buffer", 3, READ)
      .execution_callback([&](renderer& renderer, render_graph::node* node, void* user_data) {
        renderer.execute_draw_calls(node);
      })
      .end_pass();

    start_pass("shadow-map-pass", shadow_map_pass_shader_handle, render_pass::RENDER_PASS, window_size)
      .texture_resource("ambient_shadow_map", framebuffer::DEPTH, WRITE)
      .buffer_resource("model_buffer", 1, READ)
      .execution_callback([&](renderer& renderer, render_graph::node* node, void* user_data) {
        renderer.execute_draw_calls(node);
      })
      .end_pass();

#if POINT_LIGHT_SHADOW_MAPS
    start_pass("point-light-shadow-pass", shadow_map_pass_shader_handle, render_pass::RENDER_PASS, window_size)
      .texture_resource("pl_shadow_map", framebuffer::DEPTH, WRITE)
      .buffer_resource("model_buffer", 1, READ)
      .execution_callback([&](renderer& renderer, const render_graph::node* node, void* user_data) {
        shader* point_light_shader = get_pass_shader("point-light-shadow-pass");
        OTHER_ASSERT(point_light_shader != nullptr, "Point light shadow pass shader not found.");

        float near_plane = 1.0f;
        float far_plane = 10.f;
        auto window_size = renderer.get_window_size();
        glm::mat4 shadow_projection = glm::perspective(glm::radians(90.0f), (float)window_size.x / window_size.y, near_plane, far_plane);

        point_light_shader->set_uniform("far_plane", far_plane);

        for (size_t i = 0; i < get_frame_render_data()->point_lights.size() && i < gpu::kMaxPointLights; ++i) {
          const auto& light = get_frame_render_data()->point_lights[i];
          glm::vec3 light_pos = light.light_position;

          std::vector<glm::mat4> light_matrices;
          light_matrices.push_back(shadow_projection * glm::lookAt(light_pos, light_pos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
          light_matrices.push_back(shadow_projection * glm::lookAt(light_pos, light_pos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
          light_matrices.push_back(shadow_projection * glm::lookAt(light_pos, light_pos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
          light_matrices.push_back(shadow_projection * glm::lookAt(light_pos, light_pos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
          light_matrices.push_back(shadow_projection * glm::lookAt(light_pos, light_pos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
          light_matrices.push_back(shadow_projection * glm::lookAt(light_pos, light_pos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));

          OTHER_ASSERT(light_matrices.size() == 6, "Point light shadow matrices should have 6 faces.");
          for (size_t i = 0; i < light_matrices.size(); ++i) {
            point_light_shader->set_uniform("shadow_matrices[" + std::to_string(i) + "]", light_matrices[i]);
          }
          point_light_shader->set_uniform("light_pos", light_pos);

          renderer.execute_draw_calls();
        }
      })
      .end_pass();
#endif  // POINT_LIGHT_SHADOW_MAPS

    start_pass("shading-pass", shading_pass_shader_handle, render_pass::RENDER_PASS, window_size)
      .clear_color(glm::vec4(0.2f, 0.2f, 0.2f, 1.f))
      .texture_resource("color_texture", framebuffer::COLOR, READ)
      .texture_resource("normal_texture", framebuffer::COLOR, READ)
      .texture_resource("position_texture", framebuffer::COLOR, READ)
      .texture_resource("ambient_shadow_map", framebuffer::DEPTH, READ)
      // .texture_resource("point-light-shadow-cube-map", framebuffer::DEPTH, READ)
      .texture_resource("screen_texture", framebuffer::COLOR, WRITE)
      .buffer_resource("direction_light_buffer", 0, READ)
      .buffer_resource("point_light_buffer", 1, READ)
      .buffer_resource("camera_buffer", 2, READ)
      .execution_callback([&](renderer& renderer, const render_graph::node* node, void* user_data) {
        renderer.get_resource<mesh>(quad_mesh_handle).draw();
      })
      .end_pass();

#if EXTRA_DEBUG_POST_PROCESSING
    start_pass("debug-post-processing", shading_pass_shader_handle, render_pass::RENDER_PASS, window_size, /* create_framebuffer = */ false)
      .texture_resource("screen_texture", framebuffer::COLOR, READ)
      .texture_resource("debug_processed_texture", framebuffer::COLOR, WRITE)
      .execution_callback([&](renderer& renderer, const render_graph::node* node, void* user_data) {
        renderer.get_resource<mesh>(quad_mesh_handle).draw();
      })
      .end_pass();
#endif

    start_pass("to-screen", screen_shader_handle, render_pass::RENDER_PASS, window_size, /* create_framebuffer = */ false)
#if EXTRA_DEBUG_POST_PROCESSING
      .texture_resource("debug_processed_texture", framebuffer::COLOR, READ)
      .texture_resource("final_frame", framebuffer::COLOR, WRITE)
#else
      .texture_resource("screen_texture", framebuffer::COLOR, READ)
#endif
      .execution_callback([&](renderer& renderer, const render_graph::node* node, void* user_data) {
        get_pass_shader("to-screen")->set_uniform("OE_texture", 0);
        renderer.get_resource<mesh>(quad_mesh_handle).draw();
      })
      .end_pass();
  }

  namespace {

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

  }  // namespace

}  // namespace other