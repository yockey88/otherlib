/**
 * @file renderer/renderer.hpp
 */
#ifndef OTHER_RENDERER_RENDERER_RENDERER_HPP
#define OTHER_RENDERER_RENDERER_RENDERER_HPP

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include "core/logger.hpp"

#include "renderer/render_graph.hpp"
#include "renderer/renderer_backend.hpp"

#include "gpu_resource/renderer_resource.hpp"

namespace other {

  class renderer_backend;

  class renderer {
   public:
    void begin_frame();
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

    void render(const render_graph& graph);

   private:
    renderer_backend* rendering();
  };

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_RENDERER_HPP