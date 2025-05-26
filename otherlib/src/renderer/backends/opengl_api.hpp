/**
 * \file renderer/backends/opengl_api.hpp
 **/
#ifndef OTHERLIB_RENDERER_BACKENDS_OPENGL_API_HPP
#define OTHERLIB_RENDERER_BACKENDS_OPENGL_API_HPP

#include <string>

#include "renderer/renderer_resource.hpp"
#include "renderer/rendering_api.hpp"

namespace other {

  class opengl_api final : public rendering_api {
   public:
    opengl_api(void* native_window_handle)
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

    void bind_texture_resource(const resource_handle& handle) override;
    void unbind_texture_resource(const resource_handle& handle) override;

    void bind_buffer_resource(const resource_handle& handle) override;
    void unbind_buffer_resource(const resource_handle& handle) override;

    void set_shader_uniform(const resource_handle& shader, const std::string& name, int value) override;
    void set_shader_uniform(const resource_handle& shader, const std::string& name, float value) override;
    void set_shader_uniform(const resource_handle& shader, const std::string& name, const glm::vec3& value) override;
    void set_shader_uniform(const resource_handle& shader, const std::string& name, const glm::vec4& value) override;
    void set_shader_uniform(const resource_handle& shader, const std::string& name, const glm::mat4& value) override;

   private:
    struct uniform_key {
      uint64_t resource_id;
      uint64_t uniform_hash;

      constexpr auto operator<=>(const uniform_key&) const = default;
    };
    std::map<uniform_key, uint32_t> shader_uniforms;
    std::map<uint64_t, uint32_t> gpu_resources;
    std::map<uint64_t, resource_type> resource_types;

    std::map<uint64_t, std::vector<uint32_t>> in_process_resources;

    std::map<uint64_t, shader> shader_resources;

    resource* get_resource(uint64_t id) override;

    void* create_buffer_resource(uint64_t id, resource_type type) override;
    void* create_texture_resource(uint64_t id, resource_type type) override;
    void* create_shader_resource(uint64_t id, resource_type type) override;

    int32_t get_resource_handle(uint64_t id) const;

    uint32_t get_shader_uniform_location(const resource_handle& shader, const std::string& name);
  };

}  // namespace other

#endif  // OTHERLIB_RENDERER_BACKENDS_OPENGL_API_HPP