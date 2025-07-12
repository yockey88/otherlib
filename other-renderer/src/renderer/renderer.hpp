/**
 * @file renderer/renderer.hpp
 */
#ifndef OTHER_RENDERER_RENDERER_RENDERER_HPP
#define OTHER_RENDERER_RENDERER_RENDERER_HPP

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include "gpu_resource/renderer_resource.hpp"
#include "renderer/draw_command.hpp"
#include "renderer/gpu_structs.hpp"
#include "renderer/render_graph.hpp"
#include "renderer/renderer_backend.hpp"

namespace other {

  class renderer_backend;

  class camera;

  struct render_data {
    camera* primary_camera = nullptr;
    std::vector<gpu::point_light> point_lights;
    std::vector<gpu::directional_light> directional_lights;

    size_t num_draw_calls = 0;
    std::map<mesh_key, size_t> mesh_indices;
    std::vector<mesh_key> mesh_keys;
    std::vector<draw_call> draw_calls;
    std::vector<gpu::graphics_material_buffer> material_buffers;
    std::vector<gpu::model_matrix_buffer> model_buffers;
  };

  class renderer {
   public:
    struct frame_resources {
      resource_handle model_buffer;
      resource_handle material_buffer;
    };

    renderer();

    void submit_render_data(render_data* data);
    void begin_frame(frame_resources* resources);
    void end_frame();

    void execute_frame(const render_graph& graph, frame_resources* resources);

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

    void execute_draw_calls();

    void render(const render_graph& graph);

    constexpr static inline size_t kMaxDrawCalls = 1024;

   private:
    friend class render_graph;
    frame_resources current_frame_resources;
    render_data* scene_data = nullptr;

    renderer_backend* rendering();
  };

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_RENDERER_HPP