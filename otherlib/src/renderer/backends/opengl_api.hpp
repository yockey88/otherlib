/**
 * \file renderer/backends/opengl_api.hpp
 **/
#ifndef OTHERLIB_RENDERER_BACKENDS_OPENGL_API_HPP
#define OTHERLIB_RENDERER_BACKENDS_OPENGL_API_HPP

#include <string>

#include "renderer/gpu_buffer.hpp"
#include "renderer/mesh.hpp"
#include "renderer/renderer_resource.hpp"
#include "renderer/rendering_api.hpp"
#include "renderer/shader.hpp"
#include "renderer/texture.hpp"

namespace other {

  class opengl_api final : public rendering_api {
   public:
    opengl_api(SDL_Window* native_window_handle)
        : rendering_api(native_window_handle) {}
    virtual ~opengl_api() override = default;

    void initialize() override;
    void shutdown() override;

    void initialize_ui_context() override;
    void shutdown_ui_context() override;

    void handle_event(SDL_Event* event) override;

    void set_clear_color(const glm::vec4& color) override;

    void begin_frame() override;
    void end_frame() override;

    void bind_shader_resource(const resource_handle& handle) override;
    void unbind_shader_resource(const resource_handle& handle) override;
    void compile_and_attach_source(const resource_handle& handle, const std::string& source, shader::source_type type) override;
    void finalize_shader(const resource_handle& handle) override;
    void dispatch_shader(const resource_handle& handle, const glm::ivec3& group_dims, shader::compute_barrier_type barrier_type) override;

    void bind_texture_resource(const resource_handle& handle, uint32_t index) override;
    void unbind_texture_resource(const resource_handle& handle, uint32_t index) override;
    void set_texture_filter(const resource_handle& handle, texture::filter min_filter, texture::filter mag_filter) override;
    void set_texture_wrap_mode(const resource_handle& handle, texture::wrap wrap_s, texture::wrap wrap_t = texture::wrap::CLAMP_TO_EDGE, texture::wrap wrap_r = texture::wrap::CLAMP_TO_EDGE) override;
    void upload_texture(const resource_handle& handle, texture::tex_type type, texture::format format, const glm::ivec2& img_size, void* data, size_t data_size) override;
    void bind_texture_as_image(const resource_handle& handle, uint32_t index, bool writable = false) override;

    void bind_buffer_resource(const resource_handle& handle, gpu_buffer::buf_type type) override;
    void unbind_buffer_resource(const resource_handle& handle) override;
    void bind_shader_buffer_resource(const resource_handle& handle, const resource_handle& shader_handle, const std::string& name, uint32_t binding_point, gpu_buffer::buf_type buffer_type) override;
    void buffer_data(const resource_handle& handle, uint32_t binding_point, const void* data, size_t size) override;
    void buffer_range(const resource_handle& handle, uint32_t binding_point, size_t start, size_t size, const void* data) override;

    void bind_mesh_resource(const resource_handle& handle) override;
    void unbind_mesh_resource(const resource_handle& handle) override;
    void set_mesh_vertex_attributes(const resource_handle& handle, const std::vector<mesh::attribute>& attributes) override;
    void draw_mesh(const resource_handle& handle, mesh::primitive_type prim_type, size_t vertex_count, size_t index_count = 0, mesh::attribute_type index_type = mesh::UNSIGNED_BYTE) override;

    void set_shader_uniform(const resource_handle& shader, const std::string& name, int32_t value) override;
    void set_shader_uniform(const resource_handle& shader, const std::string& name, float value) override;
    void set_shader_uniform(const resource_handle& shader, const std::string& name, const glm::vec3& value) override;
    void set_shader_uniform(const resource_handle& shader, const std::string& name, const glm::vec4& value) override;
    void set_shader_uniform(const resource_handle& shader, const std::string& name, const glm::mat4& value) override;

   private:
    std::map<uint64_t, uint32_t> gpu_resources;
    std::map<uint64_t, resource_type> resource_types;
    std::map<uint64_t, std::vector<uint32_t>> in_process_resources;

    std::map<uint64_t, shader> shader_resources;
    struct uniform_key {
      uint64_t resource_id;
      uint64_t uniform_hash;

      constexpr auto operator<=>(const uniform_key&) const = default;
    };
    std::map<uniform_key, uint32_t> shader_uniforms;

    std::map<uint64_t, texture> texture_resources;
    std::map<uint64_t, gpu_buffer> buffer_resources;
    std::map<uint64_t, mesh> mesh_resources;

    mesh* create_mesh_resource(const resource_handle& handle, resource_type type) override;
    void destroy_mesh_resource(const resource_handle& handle) override;

    gpu_buffer* create_buffer_resource(const resource_handle& handle, resource_type type) override;
    void destroy_buffer_resource(const resource_handle& handle) override;

    texture* create_texture_resource(const resource_handle& handle, resource_type type) override;
    void destroy_texture_resource(const resource_handle& handle) override;

    shader* create_shader_resource(const resource_handle& handle, resource_type type) override;
    void destroy_shader_resource(const resource_handle& handle) override;

    int32_t get_gl_texture_type(texture::tex_type type) const;
    int32_t get_gl_texture_format(texture::format format) const;
    int32_t get_gl_texture_channel_format(texture::format format) const;
    int32_t get_gl_texture_format_type(texture::format format) const;
    int32_t get_gl_texture_filter(texture::filter filter) const;
    int32_t get_gl_texture_wrap_mode(texture::wrap wrap) const;

    int32_t get_gl_buffer_type(gpu_buffer::buf_type type) const;
    int32_t get_gl_buffer_usage(gpu_buffer::usage usage) const;
    // int32_t get_gl_buffer_access(gpu_buffer::access access) const;

    int32_t get_gl_attr_type(mesh::attribute_type type) const;
    int32_t get_gl_attr_size(mesh::attribute_type type) const;

    int32_t get_gl_prim_type(mesh::primitive_type type) const;

    int32_t get_resource_handle(uint64_t id) const;
    uint32_t get_shader_uniform_location(const resource_handle& shader, const std::string& name);
  };

}  // namespace other

#endif  // OTHERLIB_RENDERER_BACKENDS_OPENGL_API_HPP