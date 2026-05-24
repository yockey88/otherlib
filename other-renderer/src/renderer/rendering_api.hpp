/**
 * \file src/renderer/rendering_api.hpp
 **/
#ifndef OTHER_RENDERING_API_HPP
#define OTHER_RENDERING_API_HPP

#include <map>
#include <string>

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include "core/scope.hpp"

#include "gpu_resource/cube_map.hpp"
#include "gpu_resource/framebuffer.hpp"
#include "gpu_resource/gpu_buffer.hpp"
#include "gpu_resource/mesh.hpp"
#include "gpu_resource/renderer_resource.hpp"
#include "gpu_resource/shader.hpp"
#include "gpu_resource/texture.hpp"
#include "model/vertex.hpp"
#include "renderer/draw_command.hpp"
#include "renderer/pipeline_definition.hpp"
#include "renderer/render_pass.hpp"
#include "renderer/window_manager.hpp"

namespace other {

  struct pass_begin_info {
    opt<resource_handle> framebuffer;  // none = swapchain
    glm::ivec2 render_area_size;
    opt<glm::vec4> clear_color;
    opt<float> clear_depth;
    render_pass::type pass_type;
  };

  struct binding_record {
    uint32_t binding_point;
    binding_type type;
    resource_handle handle;
    size_t offset = 0;  // for PER_DRAW_CALL dynamic offsets
    size_t size = 0;    // 0 = whole resource
  };

  class rendering_api {
   public:
    rendering_api() = default;
    virtual ~rendering_api() {
      window_mgr = nullptr;
    }

    SDL_Window* window_handle();
    void* get_context_handle();

    void initialize(scope<window_manager> window_mgr);
    void shutdown();

    scope<window_manager>& get_window_manager() {
      return window_mgr;
    }

    virtual void on_initialize(scope<window_manager>& window_mgr) = 0;
    virtual void on_shutdown(scope<window_manager>& window_mgr) = 0;

    void destroy_windows();

    virtual void initialize_ui_context() = 0;
    virtual void shutdown_ui_context() = 0;

    virtual void handle_event(SDL_Event* event) = 0;

    virtual void set_clear_color(const glm::vec4& color) = 0;

    void begin_frame();
    void end_frame();

    virtual void on_begin_frame(scope<window_manager>& window_mgr) = 0;
    virtual void on_end_frame(scope<window_manager>& window_mgr) = 0;

    virtual void begin_pass(const pass_begin_info& info) = 0;
    virtual void end_pass() = 0;

    virtual void bind_set(uint32_t set_index, std::span<const binding_record> records) = 0;
    virtual void set_dynamic_offsets(uint32_t set_index, std::span<const uint32_t> offsets) = 0;

    virtual void execute_draw_call(render_polygon_mode render_state, mesh::primitive_type draw_mode, const draw_call& call) = 0;

    void begin_ui_frame();
    void end_ui_frame();

    virtual void begin_ui_frame_backend_newframe() = 0;
    virtual void end_ui_frame_backend_draw_data() = 0;

    virtual void bind_shader_resource(const resource_handle& handle) = 0;
    virtual void unbind_shader_resource(const resource_handle& handle) = 0;
    virtual void compile_and_attach_source(const resource_handle& handle, const std::string_view source, shader::source_type type) = 0;
    virtual void finalize_shader(const resource_handle& handle) = 0;
    virtual void dispatch_shader(const resource_handle& handle, const glm::ivec3& group_dims, shader::compute_barrier_type barrier_type) = 0;

    virtual void bind_texture_resource(const resource_handle& handle, uint32_t index) = 0;
    virtual void unbind_texture_resource(const resource_handle& handle, uint32_t index) = 0;
    virtual void set_texture_filter(const resource_handle& handle, texture::filter min_filter, texture::filter mag_filter) = 0;
    virtual void set_texture_wrap_mode(const resource_handle& handle, texture::wrap wrap_s, texture::wrap wrap_t = texture::wrap::CLAMP_TO_EDGE, texture::wrap wrap_r = texture::wrap::CLAMP_TO_EDGE) = 0;
    virtual void upload_texture(const resource_handle& handle, texture::tex_type type, texture::format format, const glm::ivec2& img_size, void* data, size_t data_size) = 0;
    virtual void bind_image(const resource_handle& handle, uint32_t index, uint32_t level, bool layered, int32_t layer = 0, texture::format frmt = texture::format::RGBA32F, access_flags flags = access_flags::READ_WRITE) = 0;
    virtual void* get_texture_gpu_resource(const resource_handle& handle) = 0;

    virtual void bind_buffer_resource(const resource_handle& handle, gpu_buffer::buf_type type) = 0;
    virtual void unbind_buffer_resource(const resource_handle& handle) = 0;
    virtual void bind_shader_buffer_resource(const resource_handle& handle, const resource_handle& shader_handle, const std::string_view name, uint32_t binding_point, gpu_buffer::buf_type buffer_type, const void* data, size_t size) = 0;
    virtual void buffer_data(const resource_handle& handle, uint32_t binding_point, const void* data, size_t size) = 0;
    virtual void buffer_range(const resource_handle& handle, uint32_t binding_point, size_t start, size_t size, const void* data) = 0;

    virtual void bind_mesh_resource(const resource_handle& handle) = 0;
    virtual void unbind_mesh_resource(const resource_handle& handle) = 0;
    virtual void set_mesh_vertex_attributes(const resource_handle& handle, const std::vector<vertex_attribute>& attributes) = 0;
    virtual void draw_mesh(const resource_handle& handle, mesh::primitive_type prim_type, size_t vertex_count, size_t index_count = 0, mesh::attribute_type index_type = mesh::UNSIGNED_BYTE) = 0;
    virtual void draw_mesh_instanced(const resource_handle& handle, const draw_call& call) = 0;

    virtual void bind_framebuffer_resource(const resource_handle& handle) = 0;
    virtual void unbind_framebuffer_resource(const resource_handle& handle) = 0;
    virtual void framebuffer_texture_2d(const resource_handle& handle, const resource_handle& texture, framebuffer::attachment_type type, uint32_t mip_level, uint32_t color_attachment_index = 0) = 0;
    virtual void finalize_framebuffer(const resource_handle& handle) = 0;

    virtual void set_shader_uniform(const resource_handle& shader, const std::string_view name, int8_t value) = 0;
    virtual void set_shader_uniform(const resource_handle& shader, const std::string_view name, uint8_t value) = 0;
    virtual void set_shader_uniform(const resource_handle& shader, const std::string_view name, int16_t value) = 0;
    virtual void set_shader_uniform(const resource_handle& shader, const std::string_view name, uint16_t value) = 0;
    virtual void set_shader_uniform(const resource_handle& shader, const std::string_view name, int32_t value) = 0;
    virtual void set_shader_uniform(const resource_handle& shader, const std::string_view name, uint32_t value) = 0;
    virtual void set_shader_uniform(const resource_handle& shader, const std::string_view name, int64_t value) = 0;
    virtual void set_shader_uniform(const resource_handle& shader, const std::string_view name, uint64_t value) = 0;
    virtual void set_shader_uniform(const resource_handle& shader, const std::string_view name, real_t value) = 0;
    virtual void set_shader_uniform(const resource_handle& shader, const std::string_view name, const glm::vec3& value) = 0;
    virtual void set_shader_uniform(const resource_handle& shader, const std::string_view name, const glm::vec4& value) = 0;
    virtual void set_shader_uniform(const resource_handle& shader, const std::string_view name, const glm::mat4& value, bool transpose = false) = 0;

    virtual uint32_t uniform_buffer_offset_alignment() const = 0;
    virtual uint32_t storage_buffer_offset_alignment() const = 0;

    resource_handle create_resource(const std::string_view name, resource_type type);
    void destroy_resource(const resource_handle& handle);

    void set_resource_name(const resource_handle& handle, const std::string_view name);

    bool resource_exists(const resource_handle& handle);

    template <typename T>
    T* get_resource_as(const resource_handle& handle) {
      return (T*)get_resource(handle.id);
    }
    resource* get_resource(natural_t id);
    std::string get_resource_name(const resource_handle& handle) const;

   protected:
    /// there is probably a better name for this since not only is this not ever gonna be a native window
    ///   but it is in fact always an SDL_Window*
    /// we also need to consider the fact that some rendering APIs may want to manage the window themselves?
    /// this is very tough though since we want to have a common window manager interface for all rendering APIs
    ///   and also may want to have more than one window per application in the future
    SDL_Window* native_window();
    void set_gpu_context(void* context);

    void* get_gpu_context() const;

    glm::vec3 get_clear_color() const;
    void override_clear_color(const glm::vec3& color);

    glm::ivec2 get_window_size() const;
    void set_window_size(const glm::ivec2& size);

    inline natural_t get_next_resource_id() {
      static natural_t next_id = 0;
      /// start at 1
      return ++next_id;
    }

    virtual int32_t get_gpu_api_window_flags() const = 0;

    virtual framebuffer* create_framebuffer_resource(const resource_handle& handle, resource_type type) = 0;
    virtual void destroy_framebuffer_resource(const resource_handle& handle) = 0;

    virtual mesh* create_mesh_resource(const resource_handle& handle, resource_type type) = 0;
    virtual void destroy_mesh_resource(const resource_handle& handle) = 0;

    virtual gpu_buffer* create_buffer_resource(const resource_handle& handle, resource_type type) = 0;
    virtual void destroy_buffer_resource(const resource_handle& handle) = 0;

    virtual texture* create_texture_resource(const resource_handle& handle, resource_type type) = 0;
    virtual void destroy_texture_resource(const resource_handle& handle) = 0;

    virtual cube_map* create_cube_map_resource(const resource_handle& handle, resource_type type) = 0;
    virtual void destroy_cube_map_resource(const resource_handle& handle) = 0;

    virtual shader* create_shader_resource(const resource_handle& handle, resource_type type) = 0;
    virtual void destroy_shader_resource(const resource_handle& handle) = 0;

    std::map<natural_t, resource_handle> resource_handles;

   private:
    void* gpu_context = nullptr;
    scope<window_manager> window_mgr;

    glm::vec3 clear_color;
    glm::ivec2 window_size;

    std::map<natural_t, resource*> resources;
    std::map<natural_t, std::string> resource_names;
  };

}  // namespace other

#endif  // OTHER_RENDERING_API_HPP