/**
 * @file renderer/backends/opengl_api.hpp
 */
#ifndef OTHER_RENDERER_RENDERER_BACKENDS_OPENGL_API_HPP
#define OTHER_RENDERER_RENDERER_BACKENDS_OPENGL_API_HPP

#include <string>

#include "gpu_resource/framebuffer.hpp"
#include "gpu_resource/gpu_buffer.hpp"
#include "gpu_resource/mesh.hpp"
#include "gpu_resource/renderer_resource.hpp"
#include "gpu_resource/shader.hpp"
#include "gpu_resource/texture.hpp"
#include "model/vertex.hpp"
#include "renderer/rendering_api.hpp"

namespace other {

  class opengl_api final : public rendering_api {
   public:
    opengl_api() : rendering_api() {}
    virtual ~opengl_api() override;

    void on_initialize(scope<window_manager>& window_mgr) override;
    void on_shutdown(scope<window_manager>& window_mgr) override;

    void initialize_ui_context() override;
    void shutdown_ui_context() override;

    void handle_event(SDL_Event* event) override;

    void set_clear_color(const glm::vec4& color) override;

    void on_begin_frame(scope<window_manager>& window_mgr) override;
    void on_end_frame(scope<window_manager>& window_mgr) override;

    void begin_pass(const pass_begin_info& info) override;
    void end_pass() override;

    void bind_set(uint32_t set_index, std::span<const binding_record> records) override;
    void set_dynamic_offsets(uint32_t, std::span<const uint32_t>) override;

    void execute_draw_call(render_polygon_mode render_state, mesh::primitive_type draw_mode, const draw_call& call) override;

    void begin_ui_frame_backend_newframe() override;
    void end_ui_frame_backend_draw_data() override;

    void bind_shader_resource(const resource_handle& handle) override;
    void unbind_shader_resource(const resource_handle& handle) override;
    void compile_and_attach_source(const resource_handle& handle, const std::string_view source, shader::source_type type) override;
    void finalize_shader(const resource_handle& handle) override;
    void dispatch_shader(const resource_handle& handle, const glm::ivec3& group_dims, shader::compute_barrier_type barrier_type) override;

    void bind_texture_resource(const resource_handle& handle, uint32_t index) override;
    void unbind_texture_resource(const resource_handle& handle, uint32_t index) override;
    void set_texture_filter(const resource_handle& handle, texture::filter min_filter, texture::filter mag_filter) override;
    void set_texture_wrap_mode(const resource_handle& handle, texture::wrap wrap_s, texture::wrap wrap_t = texture::wrap::CLAMP_TO_EDGE, texture::wrap wrap_r = texture::wrap::CLAMP_TO_EDGE) override;
    void upload_texture(const resource_handle& handle, texture::tex_type type, texture::format format, const glm::ivec2& img_size, uint32_t depth, void* data, size_t data_size) override;
    void bind_image(const resource_handle& handle, uint32_t index, uint32_t level, bool layered, int32_t layer, texture::format frmt, access_flags flags) override;
    void* get_texture_gpu_resource(const resource_handle& handle) override;

    void bind_buffer_resource(const resource_handle& handle, gpu_buffer::buf_type type) override;
    void unbind_buffer_resource(const resource_handle& handle) override;
    void bind_shader_buffer_resource(const resource_handle& handle, const resource_handle& shader_handle, const std::string_view name, uint32_t binding_point, gpu_buffer::buf_type buffer_type, const void* data, size_t size) override;
    void buffer_data(const resource_handle& handle, uint32_t binding_point, const void* data, size_t size) override;
    void buffer_range(const resource_handle& handle, uint32_t binding_point, size_t start, size_t size, const void* data) override;

    void bind_mesh_resource(const resource_handle& handle) override;
    void unbind_mesh_resource(const resource_handle& handle) override;
    void set_mesh_vertex_attributes(const resource_handle& handle, const std::vector<vertex_attribute>& attributes) override;
    void draw_mesh(const resource_handle& handle, mesh::primitive_type prim_type, size_t vertex_count, size_t index_count = 0, mesh::attribute_type index_type = mesh::UNSIGNED_BYTE) override;
    void draw_mesh_instanced(const resource_handle& handle, const draw_call& call) override;

    void bind_framebuffer_resource(const resource_handle& handle) override;
    void unbind_framebuffer_resource(const resource_handle& handle) override;
    void framebuffer_texture_2d(const resource_handle& handle, const resource_handle& texture, framebuffer::attachment_type type, uint32_t mip_level, uint32_t color_attachment_index = 0) override;
    void finalize_framebuffer(const resource_handle& handle) override;

    void set_shader_uniform(const resource_handle& shader, const std::string_view name, int8_t value) override;
    void set_shader_uniform(const resource_handle& shader, const std::string_view name, uint8_t value) override;
    void set_shader_uniform(const resource_handle& shader, const std::string_view name, int16_t value) override;
    void set_shader_uniform(const resource_handle& shader, const std::string_view name, uint16_t value) override;
    void set_shader_uniform(const resource_handle& shader, const std::string_view name, int32_t value) override;
    void set_shader_uniform(const resource_handle& shader, const std::string_view name, uint32_t value) override;
    void set_shader_uniform(const resource_handle& shader, const std::string_view name, int64_t value) override;
    void set_shader_uniform(const resource_handle& shader, const std::string_view name, uint64_t value) override;
    void set_shader_uniform(const resource_handle& shader, const std::string_view name, real_t value) override;
    void set_shader_uniform(const resource_handle& shader, const std::string_view name, const glm::vec3& value) override;
    void set_shader_uniform(const resource_handle& shader, const std::string_view name, const glm::vec4& value) override;
    void set_shader_uniform(const resource_handle& shader, const std::string_view name, const glm::mat4& value, bool transpose = false) override;

    uint32_t uniform_buffer_offset_alignment() const override;
    uint32_t storage_buffer_offset_alignment() const override;

   private:
    std::map<natural_t, uint32_t> gpu_resources;
    std::map<natural_t, resource_type> resource_types;
    std::map<natural_t, std::vector<uint32_t>> in_process_resources;

    std::map<natural_t, shader> shader_resources;
    struct uniform_key {
      natural_t resource_id;
      natural_t uniform_hash;

      constexpr auto operator<=>(const uniform_key&) const = default;
    };
    std::map<uniform_key, uint32_t> shader_uniforms;

    std::map<natural_t, texture> texture_resources;
    std::map<natural_t, cube_map> cube_map_resources;
    std::map<natural_t, std::array<uint32_t, cube_map::kCubeFaces>> cube_map_faces;

    struct shader_binding {
      uint32_t buffer_id;
      uint32_t shader_id;

      constexpr auto operator<=>(const shader_binding&) const = default;
    };
    std::map<shader_binding, natural_t> shader_block_bindings;
    std::map<natural_t, gpu_buffer> buffer_resources;

    std::map<natural_t, mesh> mesh_resources;

    std::map<natural_t, framebuffer> framebuffer_resources;
    std::map<natural_t, uint32_t> framebuffer_renderbuffers;

    int32_t get_gpu_api_window_flags() const override;

    framebuffer* create_framebuffer_resource(const resource_handle& handle, resource_type type) override;
    void destroy_framebuffer_resource(const resource_handle& handle) override;

    mesh* create_mesh_resource(const resource_handle& handle, resource_type type) override;
    void destroy_mesh_resource(const resource_handle& handle) override;

    gpu_buffer* create_buffer_resource(const resource_handle& handle, resource_type type) override;
    void destroy_buffer_resource(const resource_handle& handle) override;

    texture* create_texture_resource(const resource_handle& handle, resource_type type) override;
    void destroy_texture_resource(const resource_handle& handle) override;

    cube_map* create_cube_map_resource(const resource_handle& handle, resource_type type) override;
    void destroy_cube_map_resource(const resource_handle& handle) override;

    shader* create_shader_resource(const resource_handle& handle, resource_type type) override;
    void destroy_shader_resource(const resource_handle& handle) override;

    int32_t get_gl_access_flags(access_flags flags) const;

    int32_t get_gl_render_polygon_mode(render_polygon_mode mode) const;

    int32_t get_gl_texture_type(texture::tex_type type) const;
    int32_t get_gl_texture_format(texture::format format) const;
    int32_t get_gl_texture_channel_format(texture::format format) const;
    int32_t get_gl_texture_format_type(texture::format format) const;
    int32_t get_gl_texture_filter(texture::filter filter) const;
    int32_t get_gl_texture_wrap_mode(texture::wrap wrap) const;

    int32_t get_gl_buffer_type(gpu_buffer::buf_type type) const;
    int32_t get_gl_buffer_usage(gpu_buffer::usage usage) const;
    // int32_t get_gl_buffer_access(gpu_buffer::access access) const;

    int32_t get_gl_barrier_mask(shader::compute_barrier_type barrier_type) const;

    int32_t get_gl_attr_type(mesh::attribute_type type) const;
    int32_t get_gl_attr_size(mesh::attribute_type type) const;
    int32_t get_gl_prim_type(mesh::primitive_type type) const;

    int32_t get_gl_fb_attachment_type(framebuffer::attachment_type type) const;
    int32_t get_gl_clear_bits(int32_t mask) const;

    int32_t get_resource_handle(natural_t id) const;
    uint32_t get_shader_uniform_location(const resource_handle& shader, const std::string_view name);
  };

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_BACKENDS_OPENGL_API_HPP