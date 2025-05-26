/**
 * \file src/renderer/rendering_api.hpp
 **/
#ifndef OTHER_RENDERING_API_HPP
#define OTHER_RENDERING_API_HPP

#include <map>
#include <string>

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include "core/logger.hpp"
#include "renderer/renderer_resource.hpp"
#include "renderer/shader.hpp"

namespace other {

  class rendering_api {
   public:
    rendering_api(void* native_window_handle)
        : native_window_handle(native_window_handle) {}
    virtual ~rendering_api() = default;

    template <typename T>
    const T window_handle() {
      return native_window<T>();
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

    virtual void bind_shader_resource(const resource_handle& handle) = 0;
    virtual void unbind_shader_resource(const resource_handle& handle) = 0;
    virtual void compile_and_attach_source(const resource_handle& handle, const std::string& source, shader::source_type type) = 0;
    virtual void finalize_shader(const resource_handle& handle) = 0;

    virtual void bind_texture_resource(const resource_handle& handle) = 0;
    virtual void unbind_texture_resource(const resource_handle& handle) = 0;

    virtual void bind_buffer_resource(const resource_handle& handle) = 0;
    virtual void unbind_buffer_resource(const resource_handle& handle) = 0;

    virtual void set_shader_uniform(const resource_handle& shader, const std::string& name, int value) = 0;
    virtual void set_shader_uniform(const resource_handle& shader, const std::string& name, float value) = 0;
    virtual void set_shader_uniform(const resource_handle& shader, const std::string& name, const glm::vec3& value) = 0;
    virtual void set_shader_uniform(const resource_handle& shader, const std::string& name, const glm::vec4& value) = 0;
    virtual void set_shader_uniform(const resource_handle& shader, const std::string& name, const glm::mat4& value) = 0;

    resource_handle create_resource(resource_type type);

    void set_resource_name(const resource_handle& handle, const std::string& name);
    bool resource_has_name(const resource_handle& handle, uint64_t name_hash) const;

    template <typename T>
    T* bind_resource_as(const resource_handle& handle) {
      return (T*)get_resource(handle.id);
    }

   protected:
    template <typename T>
    T native_window() {
      return static_cast<T>(native_window_handle);
    }

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

    inline uint64_t get_next_resource_id() {
      static uint64_t next_id = 0;
      /// start at 1
      return ++next_id;
    }

    virtual resource* get_resource(uint64_t id) = 0;

    virtual void* create_buffer_resource(uint64_t id, resource_type type) = 0;
    virtual void* create_texture_resource(uint64_t id, resource_type type) = 0;
    virtual void* create_shader_resource(uint64_t id, resource_type type) = 0;

   private:
    void* gpu_context = nullptr;
    void* native_window_handle = nullptr;

    glm::vec3 clear_color;

    std::map<uint64_t, resource_handle> resource_handles;
    std::map<uint64_t, resource*> resources;

    struct resource_name {
      std::string name;
      uint64_t hash;

      constexpr auto operator<=>(const resource_name&) const = default;
    };
    std::map<uint64_t, resource_name> resource_names;
  };

}  // namespace other

#endif  // OTHER_RENDERING_API_HPP