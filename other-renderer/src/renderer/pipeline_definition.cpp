/**
 * \file renderer/pipeline_definition.cpp
 **/
#include "renderer/pipeline_definition.hpp"

#include "renderer/gpu_structs.hpp"

namespace other {

  executor_type executor_type_from_string(const std::string_view str) {
    natural_t hash = FNV(str);
    switch (hash) {
      case FNV("draw_scene"): return executor_type::DRAW_SCENE;
      case FNV("fullscreen_quad"): return executor_type::FULLSCREEN_QUAD;
      case FNV("compute_dispatch"): return executor_type::COMPUTE_DISPATCH;
      case FNV("noop"): return executor_type::NOOP;
      case FNV("script"): return executor_type::SCRIPT;
      default: return executor_type::NOOP;
    }
  }

  std::string_view executor_type_to_string(executor_type type) {
    switch (type) {
      case executor_type::DRAW_SCENE: return "draw_scene";
      case executor_type::FULLSCREEN_QUAD: return "fullscreen_quad";
      case executor_type::COMPUTE_DISPATCH: return "compute_dispatch";
      case executor_type::NOOP: return "noop";
      case executor_type::SCRIPT: return "script";
      default: return "noop";
    }
  }

  pipeline_definition get_basic_geometry_only_pipeline() {
    pipeline_definition def;
    def.name = "basic-geometry-only";
    def.version = 1;

    def.buffers = {
      { .name = "material_buffer", .type = gpu_buffer::buf_type::STORAGE_BUFFER, .usage = gpu_buffer::usage::DYNAMIC, .tag = resource_tag(resource_tag::kMaterialTag) },
      { .name = "camera_buffer", .type = gpu_buffer::buf_type::UNIFORM_BUFFER, .usage = gpu_buffer::usage::DYNAMIC, .tag = resource_tag(resource_tag::kCameraTag) },
      { .name = "model_buffer", .type = gpu_buffer::buf_type::UNIFORM_BUFFER, .usage = gpu_buffer::usage::DYNAMIC, .tag = resource_tag(resource_tag::kModelTag) },
      { .name = "bone_buffer", .type = gpu_buffer::buf_type::UNIFORM_BUFFER, .usage = gpu_buffer::usage::DYNAMIC, .tag = resource_tag(resource_tag::kBoneTag) },
    };

    def.textures = {
      { .name = "color_texture", .use_window_size = true, .type = texture::tex_type::TEXTURE_2D, .format = texture::format::RGBA32U },
      { .name = "normal_texture", .use_window_size = true, .type = texture::tex_type::TEXTURE_2D, .format = texture::format::RGBA16F },
      { .name = "position_texture", .use_window_size = true, .type = texture::tex_type::TEXTURE_2D, .format = texture::format::RGBA16F },
      { .name = "screen_texture", .use_window_size = true, .type = texture::tex_type::TEXTURE_2D, .format = texture::format::RGBA16F, .tag = resource_tag(resource_tag::kScreenTag) },
    };

    const std::vector<shader::setting> std_defines = {
      { "MAX_OBJECTS", std::to_string(gpu::kMaxObjects) },
      { "MAX_VERTEX_BONE_INFLUENCE", "4" },
      { "MAX_BONES", std::to_string(gpu::kMaxObjects) },
    };

    def.shaders = {
      {
        .name = "geometry_pass_shader",
        .vertex_path = "resources/basic-instancing-gbuffer.vert",
        .fragment_path = "resources/basic-instancing-gbuffer.frag",
        .defines = std_defines,
      },
      {
        .name = "screen_shader",
        .vertex_path = "resources/basic-textured-quad.vert",
        .fragment_path = "resources/basic-textured-quad.frag",
      },
    };

    def.passes = {
      // geometry pass, fix this to only write color texture
      {
        .name = "geometry-pass",
        .pass_type = render_pass::RENDER_PASS,
        .shader_name = "geometry_pass_shader",
        .inputs = {
          { .resource_name = "material_buffer", .binding = 0 },
          { .resource_name = "model_buffer", .binding = 1 },
          { .resource_name = "camera_buffer", .binding = 2 },
          { .resource_name = "bone_buffer", .binding = 3 },
        },
        .outputs = {
          { .resource_name = "color_texture", .attachment = framebuffer::COLOR },
          { .resource_name = "normal_texture", .attachment = framebuffer::COLOR },
          { .resource_name = "position_texture", .attachment = framebuffer::COLOR },
        },
        .executor = { .name = "draw_scene" },
      },
      // fix this to read color_texture and output to screen_texture
      {
        .name = "to-screen",
        .pass_type = render_pass::RENDER_PASS,
        .shader_name = "screen_shader",
        .create_framebuffer = false,
        .inputs = {
          { .resource_name = "screen_texture", .attachment = framebuffer::COLOR },
        },
        .executor = {
          .name = "fullscreen_quad",
          .uniforms = {
            { "OE_texture", value(int32_t{ 0 }) },
            { "OE_exposure", value(1.0f) },
          },
        },
      }
    };

    def.required_tags = {
      resource_tag(resource_tag::kCameraTag),
      resource_tag(resource_tag::kModelTag),
    };

    return def;
  }

  pipeline_definition get_default_instancing_pipeline() {
    pipeline_definition def;
    def.shadow_map_pass_name = "shadow-map-pass";
    def.shading_pass_name = "shading-pass";
    def.light_space_matrix_uniform_name = "OE_light_space_matrix";

    def.name = "default-instancing";
    def.version = 1;

    def.buffers = {
      { .name = "camera_buffer", .type = gpu_buffer::buf_type::UNIFORM_BUFFER, .usage = gpu_buffer::usage::DYNAMIC, .tag = resource_tag(resource_tag::kCameraTag) },
      { .name = "model_buffer", .type = gpu_buffer::buf_type::UNIFORM_BUFFER, .usage = gpu_buffer::usage::DYNAMIC, .tag = resource_tag(resource_tag::kModelTag) },
      { .name = "bone_buffer", .type = gpu_buffer::buf_type::UNIFORM_BUFFER, .usage = gpu_buffer::usage::DYNAMIC, .tag = resource_tag(resource_tag::kBoneTag) },
      { .name = "point_light_buffer", .type = gpu_buffer::buf_type::STORAGE_BUFFER, .usage = gpu_buffer::usage::DYNAMIC, .tag = resource_tag(resource_tag::kPointLightTag) },
      { .name = "direction_light_buffer", .type = gpu_buffer::buf_type::STORAGE_BUFFER, .usage = gpu_buffer::usage::DYNAMIC, .tag = resource_tag(resource_tag::kDirectionLightTag) },
      { .name = "material_buffer", .type = gpu_buffer::buf_type::STORAGE_BUFFER, .usage = gpu_buffer::usage::DYNAMIC, .tag = resource_tag(resource_tag::kMaterialTag) },
    };

    def.textures = {
      { .name = "color_texture", .use_window_size = true, .type = texture::tex_type::TEXTURE_2D, .format = texture::format::RGBA32U },
      { .name = "normal_texture", .use_window_size = true, .type = texture::tex_type::TEXTURE_2D, .format = texture::format::RGBA16F },
      { .name = "position_texture", .use_window_size = true, .type = texture::tex_type::TEXTURE_2D, .format = texture::format::RGBA16F },
      { .name = "ambient_shadow_map", .use_window_size = true, .type = texture::tex_type::TEXTURE_2D, .format = texture::format::DEPTHF },
      { .name = "screen_texture", .use_window_size = true, .type = texture::tex_type::TEXTURE_2D, .format = texture::format::RGBA16F, .tag = resource_tag(resource_tag::kScreenTag) },
    };

    const std::vector<shader::setting> std_defines = {
      { "MAX_OBJECTS", std::to_string(gpu::kMaxObjects) },
      { "MAX_VERTEX_BONE_INFLUENCE", "4" },
      { "MAX_BONES", std::to_string(gpu::kMaxObjects) },
    };

    def.shaders = {
      {
        .name = "geometry_pass_shader",
        .vertex_path = "resources/basic-instancing-gbuffer.vert",
        .fragment_path = "resources/basic-instancing-gbuffer.frag",
        .defines = std_defines,
      },
      {
        .name = "shadow_map_shader",
        .vertex_path = "resources/basic-instancing-shadow-map.vert",
        .fragment_path = "resources/basic-instancing-shadow-map.frag",
        .defines = std_defines,
      },
      {
        .name = "shading_pass_shader",
        .vertex_path = "resources/basic-shading.vert",
        .fragment_path = "resources/basic-shading.frag",
      },
      {
        .name = "screen_shader",
        .vertex_path = "resources/basic-textured-quad.vert",
        .fragment_path = "resources/basic-textured-quad.frag",
      },
    };

    def.passes = {
      // geometry pass
      {
        .name = "geometry-pass",
        .pass_type = render_pass::RENDER_PASS,
        .shader_name = "geometry_pass_shader",
        .inputs = {
          { .resource_name = "material_buffer", .binding = 0 },
          { .resource_name = "model_buffer", .binding = 1 },
          { .resource_name = "camera_buffer", .binding = 2 },
          { .resource_name = "bone_buffer", .binding = 3 },
        },
        .outputs = {
          { .resource_name = "color_texture", .attachment = framebuffer::COLOR },
          { .resource_name = "normal_texture", .attachment = framebuffer::COLOR },
          { .resource_name = "position_texture", .attachment = framebuffer::COLOR },
        },
        .executor = { .name = "draw_scene" },
      },
      // shadow map pass
      {
        .name = "shadow-map-pass",
        .pass_type = render_pass::RENDER_PASS,
        .shader_name = "shadow_map_shader",
        .inputs = {
          { .resource_name = "model_buffer", .binding = 1 },
        },
        .outputs = {
          { .resource_name = "ambient_shadow_map", .attachment = framebuffer::DEPTH },
        },
        .executor = { .name = "draw_scene" },
      },
      // shading pass
      {
        .name = "shading-pass",
        .pass_type = render_pass::RENDER_PASS,
        .shader_name = "shading_pass_shader",
        .clear_color = glm::vec4(0.2f, 0.2f, 0.2f, 1.f),
        .inputs = {
          { .resource_name = "color_texture", .attachment = framebuffer::COLOR },
          { .resource_name = "normal_texture", .attachment = framebuffer::COLOR },
          { .resource_name = "position_texture", .attachment = framebuffer::COLOR },
          { .resource_name = "ambient_shadow_map", .attachment = framebuffer::DEPTH },
          { .resource_name = "direction_light_buffer", .binding = 0 },
          { .resource_name = "point_light_buffer", .binding = 1 },
          { .resource_name = "camera_buffer", .binding = 2 },
        },
        .outputs = {
          { .resource_name = "screen_texture", .attachment = framebuffer::COLOR },
        },
        .executor = {
          .name = "fullscreen_quad",
          .uniforms = {
            { "OE_gbuff_albedo", value(int32_t{ 0 }) },
            { "OE_gbuff_normal", value(int32_t{ 1 }) },
            { "OE_gbuff_position", value(int32_t{ 2 }) },
            { "OE_shadow_map", value(int32_t{ 3 }) },
          },
        },
      },
      {
        .name = "to-screen",
        .pass_type = render_pass::RENDER_PASS,
        .shader_name = "screen_shader",
        .create_framebuffer = false,
        .inputs = {
          { .resource_name = "screen_texture", .attachment = framebuffer::COLOR },
        },
        .executor = {
          .name = "fullscreen_quad",
          .uniforms = {
            { "OE_texture", value(int32_t{ 0 }) },
            { "OE_exposure", value(1.0f) },
          },
        },
      }
    };

    def.required_tags = {
      resource_tag(resource_tag::kCameraTag),
      resource_tag(resource_tag::kModelTag),
      resource_tag(resource_tag::kMaterialTag),
      resource_tag(resource_tag::kDirectionLightTag)
    };

    return def;
  }

  pipeline_definition get_empty_pipeline() {
    pipeline_definition def;
    def.name = "empty";
    def.version = 1;
    /// no passes, no textures, no shaders
    ///     useful for UI-only
    return def;
  }

}  // namespace other