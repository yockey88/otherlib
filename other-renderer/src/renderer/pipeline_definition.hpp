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
#include "renderer/util/pipeline_asset_validation.hpp"

namespace other {

  enum class executor_type : uint32_t {
    DRAW_SCENE,  /// renderer.execute_draw_calls(node)
    FULLSCREEN_QUAD,
    COMPUTE_DISPATCH,
    WINDOW_SIZED_COMPUTE_DISPATCH,
    VOXELIZE,
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
    uint32_t depth = 0;  // For 3D textures, store the depth separately
    texture::tex_type type = texture::tex_type::TEXTURE_2D;
    texture::format format = texture::format::RGBA16F;
    resource_tag tag = resource_tag::none();

    // 0 = full chain, 1 = no mips
    uint32_t mip_levels = 1;
    bool generate_mips = false;

    opt<filepath> seed_texture_path;

    opt<std::pair<texture::filter, texture::filter>> filters;
    opt<std::tuple<texture::wrap, texture::wrap, texture::wrap>> wraps;
  };

  struct pipeline_shader_definition {
    std::string name;
    std::string vertex_path;
    std::string fragment_path;
    opt<std::string> geometry_path;
    opt<std::string> compute_path;
    ostd::vector<shader::setting> defines;
  };

  struct pipeline_resource_reference {
    std::string resource_name;
    resource_type type;

    std::string uniform_name;  //< for samplerXD uniforms, imageXD uniforms, or bindless resource indexing
    uint32_t binding = 0;
    framebuffer::attachment_type attachment = framebuffer::COLOR;
    uint32_t mip_level = 0;
    access_flags access = access_flags::READ;
  };

  struct pipeline_executor_definition {
    std::string name = "noop";
    ostd::map<std::string, value> params;
  };

  struct pass_uniform_definition {
    std::string pass_name;
    std::string name;
    value val;
  };

  struct pipeline_pass_definition {
    std::string name;
    render_pass::type pass_type = render_pass::RENDER_PASS;
    std::string shader_name;

    bool use_window_size = true;
    glm::ivec2 fixed_size = { 0, 0 };
    bool create_framebuffer = true;
    uint32_t samples = 1;
    opt<glm::vec4> clear_color;

    ostd::vector<pipeline_resource_reference> inputs;
    ostd::vector<pipeline_resource_reference> outputs;
    pipeline_executor_definition executor;

    ostd::vector<std::string> depends_on;
    ostd::vector<frame_binding_definition> bindings;

    ostd::map<natural_t, pass_uniform_definition> uniforms;  //< FNV(pass_name + "." + name) -> value

    // for passes that need to run multiple times per frame, i.e cascaded shadow maps
    uint32_t iterations_per_frame = 1;
    // for passes that use the "draw_scene" executor, used to provision per-draw-call resources
    opt<uint32_t> expected_max_draws;

    framebuffer::clear_mask_bit clear_flags = framebuffer::ALL_BITS;
    bool override_fb_clear = false;
  };

  struct pipeline_definition {
    std::string name = "unnamed";
    uint32_t version = 1;

    std::string display_texture_name;

    ostd::vector<pipeline_buffer_definition> buffers;
    ostd::vector<pipeline_texture_definition> textures;
    ostd::vector<pipeline_shader_definition> shaders;
    ostd::vector<pipeline_pass_definition> passes;

    /// if non-empty, assert these tags are present before marking valid
    ostd::vector<resource_tag> required_tags;
  };

  executor_type executor_type_from_string(const std::string_view str);
  shader::compute_barrier_type compute_barrier_type_from_string(const std::string_view str);
  std::string_view executor_type_to_string(executor_type type);
  gpu_buffer::buf_type buffer_type_from_binding(binding_type type);

  gpu_buffer::buf_type buffer_type_from_string(const std::string_view str);
  gpu_buffer::usage buffer_usage_from_string(const std::string_view str);
  texture::tex_type texture_type_from_string(const std::string_view str);
  texture::format texture_format_from_string(const std::string_view str);
  render_pass::type render_pass_type_from_string(const std::string_view str);
  framebuffer::clear_mask_bit render_pass_clear_bits_from_strings(const std::span<const std::string> str);
  binding_scope pass_binding_scope_from_string(const std::string_view str);
  binding_type pass_binding_type_from_string(const std::string_view str);
  framebuffer::attachment_type framebuffer_attachment_type_from_string(const std::string_view str);

  resource_tag resource_tag_from_string(const std::string_view str);

  pipeline_definition read_pipeline_definition_from_file(const filepath& path);
  pipeline_definition get_empty_pipeline();

  detail::validation_result validate_pipeline_definition(const pipeline_definition& def);

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_PIPELINE_DEFINITION_HPP