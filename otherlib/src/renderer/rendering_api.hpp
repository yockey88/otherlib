/**
 * \file src/renderer/rendering_api.hpp
 **/
#ifndef OTHER_RENDERING_API_HPP
#define OTHER_RENDERING_API_HPP

#include <map>
#include <string>

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include "renderer/gpu_buffer.hpp"
#include "renderer/mesh.hpp"
#include "renderer/renderer_resource.hpp"
#include "renderer/shader.hpp"
#include "renderer/texture.hpp"

namespace other {

  class rendering_api {
   public:
    rendering_api(SDL_Window* native_window_handle)
        : native_window_handle(native_window_handle) {}
    virtual ~rendering_api() = default;

    SDL_Window* window_handle() {
      return native_window();
    }
    void* get_context_handle() {
      return native_window_handle;
    }

    virtual void initialize() = 0;
    virtual void shutdown() = 0;

    virtual void initialize_ui_context() = 0;
    virtual void shutdown_ui_context() = 0;

    virtual void handle_event(SDL_Event* event) = 0;

    virtual void set_clear_color(const glm::vec4& color) = 0;

    virtual void begin_frame() = 0;
    virtual void end_frame() = 0;

    void begin_ui_frame();
    void end_ui_frame();

    virtual void begin_ui_frame_backend_newframe() = 0;
    virtual void end_ui_frame_backend_draw_data() = 0;

    virtual void bind_shader_resource(const resource_handle& handle) = 0;
    virtual void unbind_shader_resource(const resource_handle& handle) = 0;
    virtual void compile_and_attach_source(const resource_handle& handle, const std::string& source, shader::source_type type) = 0;
    virtual void finalize_shader(const resource_handle& handle) = 0;
    virtual void dispatch_shader(const resource_handle& handle, const glm::ivec3& group_dims, shader::compute_barrier_type barrier_type) = 0;

    virtual void bind_texture_resource(const resource_handle& handle, uint32_t index) = 0;
    virtual void unbind_texture_resource(const resource_handle& handle, uint32_t index) = 0;
    virtual void set_texture_filter(const resource_handle& handle, texture::filter min_filter, texture::filter mag_filter) = 0;
    virtual void set_texture_wrap_mode(const resource_handle& handle, texture::wrap wrap_s, texture::wrap wrap_t = texture::wrap::CLAMP_TO_EDGE, texture::wrap wrap_r = texture::wrap::CLAMP_TO_EDGE) = 0;
    virtual void upload_texture(const resource_handle& handle, texture::tex_type type, texture::format format, const glm::ivec2& img_size, void* data, size_t data_size) = 0;
    virtual void bind_texture_as_image(const resource_handle& handle, uint32_t index, bool writable = false) = 0;

    virtual void bind_buffer_resource(const resource_handle& handle, gpu_buffer::buf_type type) = 0;
    virtual void unbind_buffer_resource(const resource_handle& handle) = 0;
    virtual void bind_shader_buffer_resource(const resource_handle& handle, const resource_handle& shader_handle, const std::string& name, uint32_t binding_point, gpu_buffer::buf_type buffer_type) = 0;
    virtual void buffer_data(const resource_handle& handle, uint32_t binding_point, const void* data, size_t size) = 0;
    virtual void buffer_range(const resource_handle& handle, uint32_t binding_point, size_t start, size_t size, const void* data) = 0;

    virtual void bind_mesh_resource(const resource_handle& handle) = 0;
    virtual void unbind_mesh_resource(const resource_handle& handle) = 0;
    virtual void set_mesh_vertex_attributes(const resource_handle& handle, const std::vector<mesh::attribute>& attributes) = 0;
    virtual void draw_mesh(const resource_handle& handle, mesh::primitive_type prim_type, size_t vertex_count, size_t index_count = 0, mesh::attribute_type index_type = mesh::UNSIGNED_BYTE) = 0;

    virtual void set_shader_uniform(const resource_handle& shader, const std::string& name, int32_t value) = 0;
    virtual void set_shader_uniform(const resource_handle& shader, const std::string& name, float value) = 0;
    virtual void set_shader_uniform(const resource_handle& shader, const std::string& name, const glm::vec3& value) = 0;
    virtual void set_shader_uniform(const resource_handle& shader, const std::string& name, const glm::vec4& value) = 0;
    virtual void set_shader_uniform(const resource_handle& shader, const std::string& name, const glm::mat4& value) = 0;

    resource_handle create_resource(const std::string& name, resource_type type);
    void destroy_resource(const resource_handle& handle);

    void set_resource_name(const resource_handle& handle, const std::string& name);

    template <typename T>
    T* get_resource_as(const resource_handle& handle) {
      return (T*)get_resource(handle.id);
    }
    resource* get_resource(uint64_t id);

   protected:
    SDL_Window* native_window() { return native_window_handle; }
    void set_gpu_context(void* context) {
      gpu_context = context;
    }

    void* get_gpu_context() const {
      return gpu_context;
    }

    glm::vec3 get_clear_color() const {
      return clear_color;
    }
    void override_clear_color(const glm::vec3& color) {
      clear_color = color;
    }

    glm::ivec2 get_window_size() const {
      return window_size;
    }
    void set_window_size(const glm::ivec2& size) {
      window_size = size;
      if (native_window_handle) {
        SDL_SetWindowSize(native_window_handle, size.x, size.y);
      }
    }

    inline uint64_t get_next_resource_id() {
      static uint64_t next_id = 0;
      /// start at 1
      return ++next_id;
    }

    virtual mesh* create_mesh_resource(const resource_handle& handle, resource_type type) = 0;
    virtual void destroy_mesh_resource(const resource_handle& handle) = 0;

    virtual gpu_buffer* create_buffer_resource(const resource_handle& handle, resource_type type) = 0;
    virtual void destroy_buffer_resource(const resource_handle& handle) = 0;

    virtual texture* create_texture_resource(const resource_handle& handle, resource_type type) = 0;
    virtual void destroy_texture_resource(const resource_handle& handle) = 0;

    virtual shader* create_shader_resource(const resource_handle& handle, resource_type type) = 0;
    virtual void destroy_shader_resource(const resource_handle& handle) = 0;

   private:
    void* gpu_context = nullptr;
    SDL_Window* native_window_handle = nullptr;

    glm::vec3 clear_color;
    glm::ivec2 window_size;

    std::map<uint64_t, resource_handle> resource_handles;
    std::map<uint64_t, resource*> resources;
    std::map<uint64_t, std::string> resource_names;
  };

}  // namespace other

#endif  // OTHER_RENDERING_API_HPP