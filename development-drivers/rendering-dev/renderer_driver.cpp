// /**
//  * \file renderer_driver.cpp
//  **/
#include "renderer_driver.hpp"

#include "renderer/render_graph.hpp"
#include "renderer/renderer_resource.hpp"

#include "model/vertex.hpp"
#include "model/vertex_buffer.hpp"

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

    constexpr static const char* vert_shader_src_cube = R"(
      #version 460 core

      layout (location = 0) in vec3 position;
      layout (location = 1) in vec3 normal;
      layout (location = 2) in vec3 tangent;
      layout (location = 3) in vec3 bitanget;
      layout (location = 4) in vec2 tex_coords;

      layout (std140) uniform camera_buffer {
        vec4 camera_position;
        vec4 camera_forward;

        /// near & far clip, defocus_angle padding x2
        vec4 camera_features;

        vec4 defocus_disk_u;
        vec4 defocus_disk_v;

        mat4 view_matrix;
        mat4 projection_matrix;
      };

      uniform mat4 model_matrix;

      out vec3 frag_color;

      void main() {
        gl_Position = vec4(position, 1.0);
        frag_color = normal;
      }
    )";

    constexpr static const char* frag_shader_src_cube = R"(
      #version 460 core

      in vec3 frag_color;

      out vec4 color;

      void main() {
        color = vec4(frag_color, 1.0);
      }
    )";
    static resource_handle cube_shader_handle;

    constexpr static real_t quad_vertices[] = {
      -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
      -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
      1.0, 1.0f, 0.0f, 1.0f, 1.0f,
      1.0, -1.0f, 0.0f, 1.0f, 0.0f
    };

    std::vector<vertex> get_cube_vertices();
    std::vector<index> get_cube_indices();

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

    cube = model::create_model("Cube", get_cube_vertices(), get_cube_indices());

    cam = serializer{}.read_from_file<camera>("artifacts/main_cam_data.bin");
    cam.look_from(glm::vec3(0, 0, 0));
    cam.look_at(glm::vec3(0, 0, -1));
    CORE_LOG_INFO("Camera data loaded from file: \n{}", type_data_handler<camera>::as_string("cam", cam));

    auto image_size = renderer->get_window_size();
    image_data.resize(image_size.x * image_size.y * kPixelStride);

    running = true;

    mouse.position = renderer->get_mouse_position();
    mouse.delta = glm::vec2(0.f, 0.f);
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

  }  // namespace

  void renderer_driver::run() {
    CORE_LOG_INFO("Running terminal driver...");

    CORE_LOG_INFO("      ...on gpu");

    glm::ivec2 window_size = renderer->get_window_size();

    /// works because they are static
    {
      write_materials_to_buffer(renderer->get_resource<gpu_buffer>(material_buffer_handle).set_shader_resource(6, comp_shader_handle));
      write_lambertian_to_buffer(renderer->get_resource<gpu_buffer>(lambertian_buffer_handle).set_shader_resource(7, comp_shader_handle));
      write_metal_to_buffer(renderer->get_resource<gpu_buffer>(metal_buffer_handle).set_shader_resource(8, comp_shader_handle));
      write_dielectrics_to_buffer(renderer->get_resource<gpu_buffer>(dielectrics_buffer_handle).set_shader_resource(9, comp_shader_handle));
      write_spheres_to_buffer(renderer->get_resource<gpu_buffer>(sphere_buffer_handle).set_shader_resource(1, comp_shader_handle));
      write_objects_to_buffer(renderer->get_resource<gpu_buffer>(object_buffer_handle).set_shader_resource(5, comp_shader_handle));
    }

    while (running) {
      pump_events();

      if (!running) {
        break;
      }

      gpu::scene_metadata metadata;
      metadata.window_size = glm::vec4(window_size.x, window_size.y, 0, 0);
      metadata.object_data = {
        spheres.size(),
        objects.size(),
        materials.size(),
        0
      };
      metadata.samples_per_pixel = cam.samples_per_pixel;
      metadata.max_depth = cam.max_bounce_depth;
      metadata.frame_index++;

      renderer->get_resource<gpu_buffer>(scene_metadata_handle)
        .set_shader_resource(3, comp_shader_handle)
        .set_data(&metadata, sizeof(gpu::scene_metadata))
        .finalize_buffer();

      gpu::camera_data cam_data = cam.to_gpu_data();
      renderer->get_resource<gpu_buffer>(camera_buffer_handle)
        .set_shader_resource(2, comp_shader_handle)
        .set_data(&cam_data, sizeof(gpu::camera_data))
        .finalize_buffer();

      // gpu::ray_gen_data ray_data = cam.to_ray_gen_data();
      // renderer->get_resource<gpu_buffer>(ray_buffer_handle)
      //   .set_shader_resource(4, comp_shader_handle)
      //   .set_data(&ray_data, sizeof(gpu::ray_gen_data))
      //   .finalize_buffer();

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

      renderer->begin_frame();

      /// compute pass
      // renderer->get_resource<texture>(screen_texture_handle).bind(0);
      // renderer->get_resource<shader>(comp_shader_handle)
      //   .dispatch({ cam.image_size.x, cam.image_size.y, 1 }, shader::compute_barrier_type::SHADER_IMAGE_ACCESS);
      // renderer->get_resource<texture>(screen_texture_handle).unbind(0);

      glm::vec3 pos = glm::vec3(0.f, 0.f, -1.f);
      glm::vec3 scale = glm::vec3(1.f, 1.f, 1.f);
      glm::mat4 model_matrix = glm::translate(glm::mat4(1.0f), pos) * glm::scale(glm::mat4(1.0f), scale);

      renderer->get_resource<shader>(cube_shader_handle)
        .bind();
      // .set_uniform("model_matrix", model_matrix);

      renderer->get_resource<mesh>(cube.vertex_buffer_handle).draw();
      renderer->get_resource<shader>(cube_shader_handle).unbind();

      /// final pass (render to screen)
      // renderer->get_resource<texture>(screen_texture_handle).bind(0);
      // renderer->get_resource<shader>(screen_shader_handle).bind();
      // renderer->get_resource<mesh>(quad_mesh_handle).draw();
      // renderer->get_resource<shader>(screen_shader_handle).unbind();
      // renderer->get_resource<texture>(screen_texture_handle).unbind(0);

      renderer->end_frame();
    }
  }

  void renderer_driver::on_shutdown() {
    CORE_LOG_INFO("Shutting down terminal driver...");

    if (renderer != nullptr) {
      renderer->destroy_resource(material_buffer_handle);
      renderer->destroy_resource(lambertian_buffer_handle);
      renderer->destroy_resource(metal_buffer_handle);

      renderer->destroy_resource(sphere_buffer_handle);
      renderer->destroy_resource(object_buffer_handle);

      renderer->destroy_resource(camera_buffer_handle);
      renderer->destroy_resource(scene_metadata_handle);
      renderer->destroy_resource(screen_texture_handle);
      renderer->destroy_resource(comp_shader_handle);
      renderer->destroy_resource(screen_shader_handle);
      renderer->destroy_resource(quad_mesh_handle);
    } else {
      /// cpu shutdown
    }

    renderer = nullptr;
  }

  void renderer_driver::on_event(SDL_Event* event) {
    switch (event->type) {
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        running = false;
        break;
      default:
        break;
    }
  }

  void renderer_driver::initialize_gpu() {
    CORE_LOG_INFO("      ...on gpu");
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

    auto image_size = renderer->get_window_size();
    screen_texture_handle = texture::create("screen_texture", texture::tex_type::TEXTURE_2D, texture::format::RGBA32F, image_size.x, image_size.y, true);

    const auto settings = {
      shader::setting{ "MAX_MATERIALS", std::to_string(gpu::kMaxMaterials) },
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
    comp_shader_handle = shader::create("comp_shader", "resources/raytrace.comp", settings);
    screen_shader_handle = shader::create("screen_shader", vert_shader_source, frag_shader_source);
    cube_shader_handle = shader::create("cube_shader", vert_shader_src_cube, frag_shader_src_cube);

    camera_buffer_handle = gpu_buffer::create("camera_buffer", gpu_buffer::buf_type::UNIFORM_BUFFER, gpu_buffer::usage::DYNAMIC);
    scene_metadata_handle = gpu_buffer::create("scene_metadata", gpu_buffer::buf_type::UNIFORM_BUFFER, gpu_buffer::usage::DYNAMIC);
    ray_buffer_handle = gpu_buffer::create("ray_buffer", gpu_buffer::buf_type::UNIFORM_BUFFER, gpu_buffer::usage::DYNAMIC);

    material_buffer_handle = gpu_buffer::create("material_buffer", gpu_buffer::buf_type::UNIFORM_BUFFER, gpu_buffer::usage::DYNAMIC);
    lambertian_buffer_handle = gpu_buffer::create("lambertian_buffer", gpu_buffer::buf_type::UNIFORM_BUFFER, gpu_buffer::usage::DYNAMIC);
    metal_buffer_handle = gpu_buffer::create("metal_buffer", gpu_buffer::buf_type::UNIFORM_BUFFER, gpu_buffer::usage::DYNAMIC);
    dielectrics_buffer_handle = gpu_buffer::create("dielectric_buffer", gpu_buffer::buf_type::UNIFORM_BUFFER, gpu_buffer::usage::DYNAMIC);
    sphere_buffer_handle = gpu_buffer::create("sphere_buffer", gpu_buffer::buf_type::UNIFORM_BUFFER, gpu_buffer::usage::DYNAMIC);
    object_buffer_handle = gpu_buffer::create("object_buffer", gpu_buffer::buf_type::UNIFORM_BUFFER, gpu_buffer::usage::DYNAMIC);

    SDL_HideCursor();
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
      indices.resize(8);
      /// top face
      indices[0] = {
        //   [edges]
        0, 1,  // (top-front)
        1      // (top-left.1)
      };
      indices[1] = {
        5,    // (top-left.2)
        5, 4  // (top-back)
      };
      indices[2] = {
        4, 0,  // (top-right)

        /// bottom face
        3,  // (bottom-front.1)
      };
      indices[3] = {
        2,    // (bottom-front.2)
        2, 6  // (bottom-left)
      };
      indices[4] = {
        6, 7,  // (bottom-back)
        7      // (bottom-right.1)
      };
      indices[5] = {
        3,  // (bottom-right.2)

        /// front face
        0, 3  // (front-right)
      };
      indices[6] = {
        1, 2,  // (front-left)

        /// left face
        5,  // (back-left.1)
      };
      indices[7] = {
        6,  // (back-left.2)

        /// back face
        4, 7  // (back-right)
      };
      return indices;
    }

  }  // namespace

}  // namespace other