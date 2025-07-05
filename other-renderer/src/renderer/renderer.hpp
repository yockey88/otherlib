/**
 * @file renderer/renderer.hpp
 */
#ifndef OTHER_RENDERER_RENDERER_RENDERER_HPP
#define OTHER_RENDERER_RENDERER_RENDERER_HPP

#include <array>

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include "core/defines.hpp"
#include "core/logger.hpp"
#include "core/memory_pool.hpp"

#include "gpu_resource/renderer_resource.hpp"
#include "renderer/draw_command.hpp"
#include "renderer/gpu_structs.hpp"
#include "renderer/render_graph.hpp"
#include "renderer/renderer_backend.hpp"

namespace other {

  class renderer_backend;

  class renderer {
   public:
    struct frame_resources {
      resource_handle model_buffer;
      resource_handle material_buffer;
    };

    renderer();

    void begin_frame(frame_resources* resources = nullptr);
    void end_frame();

    void begin_ui_frame();
    void end_ui_frame();

    glm::ivec2 get_window_size();
    void set_clear_color(const glm::vec4& color);

    glm::vec2 get_mouse_position();

    resource_handle create_resource(const std::string& name, resource_type type);
    void destroy_resource(const resource_handle& handle);

    template <typename T>
    T& get_resource(const resource_handle& handle) {
      return *rendering()->api()->get_resource_as<T>(handle);
    }

    void submit_model(model* draw_model, shader* shader_handle, const gpu::graphics_material& material, const glm::mat4& root_transform = glm::mat4(1.0f));
    void submit_draw_command(const draw_command& command);

    void execute_draw_calls();

    void render(const render_graph& graph);

   private:
    frame_resources current_frame_resources;

    resource_handle model_buffer_handle;
    resource_handle material_buffer_handle;

    std::map<mesh_key, integer_t> mesh_indices;
    memory_pool<mesh_key> mesh_key_pool;

    integer_t num_draw_calls = 0;
    memory_pool<draw_call> draw_call_pool;
    memory_pool<arena_buffer> material_buffer_pool;
    memory_pool<arena_buffer> model_buffer_pool;

    integer_t get_mesh_key_index(const draw_command& key);

    renderer_backend* rendering();
  };

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_RENDERER_HPP