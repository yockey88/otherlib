/**
 * \file renderer/pipeline_definition.hpp
 **/
#ifndef OTHER_RENDERER_RENDERER_PIPELINE_DEFINITION_HPP
#define OTHER_RENDERER_RENDERER_PIPELINE_DEFINITION_HPP

#include "core/defines.hpp"
#include "core/value.hpp"

#include "gpu_resource/framebuffer.hpp"
#include "gpu_resource/gpu_buffer.hpp"
#include "gpu_resource/shader.hpp"
#include "gpu_resource/texture.hpp"
#include "renderer/frame_binding_definition.hpp"
#include "renderer/render_pass.hpp"
#include "renderer/resource_tag.hpp"

namespace other {

  enum class executor_type : uint32_t {
    DRAW_SCENE,  /// renderer.execute_draw_calls(node)
    FULLSCREEN_QUAD,
    COMPUTE_DISPATCH,
    NOOP,  /// used for resource transitions that don't issue draw calls or dispatches
    SCRIPT,
  };

  struct pipeline_buffer_definition {
    std::string name;
    gpu_buffer::buf_type type = gpu_buffer::buf_type::UNIFORM_BUFFER;
    gpu_buffer::usage usage = gpu_buffer::usage::DYNAMIC;
    resource_tag tag = resource_tag::none();
  };

  struct pipeline_texture_definition {
    std::string name;
    bool use_window_size = true;
    glm::ivec2 fixed_size = { 1080, 720 };
    texture::tex_type type = texture::tex_type::TEXTURE_2D;
    texture::format format = texture::format::RGBA16F;
    resource_tag tag = resource_tag::none();

    opt<std::pair<texture::filter, texture::filter>> filters;
    opt<std::tuple<texture::wrap, texture::wrap, texture::wrap>> wraps;
  };

  struct pipeline_shader_definition {
    std::string name;
    std::string vertex_path;
    std::string fragment_path;
    opt<std::string> geometry_path;
    opt<std::string> compute_path;
    std::vector<shader::setting> defines;
  };

  struct pipeline_resource_reference {
    std::string resource_name;
    uint32_t binding = 0;
    framebuffer::attachment_type attachment = framebuffer::COLOR;
  };

  struct pipeline_executor_definition {
    std::string name = "noop";

    std::map<std::string, value> uniforms;
    std::map<std::string, value> params;
  };

  struct pipeline_pass_definition {
    std::string name;
    render_pass::type pass_type = render_pass::RENDER_PASS;
    std::string shader_name;

    bool use_window_size = true;
    glm::ivec2 fixed_size = { 0, 0 };
    bool create_framebuffer = true;
    opt<glm::vec4> clear_color;

    std::vector<pipeline_resource_reference> inputs;
    std::vector<pipeline_resource_reference> outputs;
    pipeline_executor_definition executor;

    std::vector<std::string> depends_on;
    std::vector<frame_binding_definition> bindings;

    // for passes that need to run multiple times per frame, i.e cascaded shadow maps
    uint32_t iterations_per_frame = 1;
    // for passes that use the "draw_scene" executor, used to provision per-draw-call resources
    opt<uint32_t> expected_max_draws;
  };

  struct pipeline_definition {
    std::string name = "unnamed";
    uint32_t version = 1;

    std::vector<pipeline_buffer_definition> buffers;
    std::vector<pipeline_texture_definition> textures;
    std::vector<pipeline_shader_definition> shaders;
    std::vector<pipeline_pass_definition> passes;

    /// if non-empty, assert these tags are present before marking valid
    std::vector<resource_tag> required_tags;

    opt<std::string> shadow_map_pass_name;
    opt<std::string> shading_pass_name;
    opt<std::string> light_space_matrix_uniform_name;
  };

  executor_type executor_type_from_string(const std::string_view str);
  shader::compute_barrier_type compute_barrier_type_from_string(const std::string_view str);
  std::string_view executor_type_to_string(executor_type type);
  gpu_buffer::buf_type buffer_type_from_binding(binding_type type);

  pipeline_definition get_basic_geometry_only_pipeline();
  pipeline_definition get_default_instancing_pipeline();
  pipeline_definition get_empty_pipeline();

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_PIPELINE_DEFINITION_HPP