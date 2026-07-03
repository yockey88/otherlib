/**
 * \file renderer/scene_draw.hpp
 **/
#ifndef OTHER_RENDERER_RENDERER_SCENE_DRAW_HPP
#define OTHER_RENDERER_RENDERER_SCENE_DRAW_HPP

#include "gpu_resource/renderer_resource.hpp"
#include "renderer/colors.hpp"

namespace other {

  class render_stream;

  class scene_draw {
   public:
    scene_draw(render_stream* stream) : stream(stream) {}

    void line(const glm::vec3& a, const glm::vec3& b, const glm::vec4& c = basic_colors::kWhite);
    void triangle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& d, const glm::vec4& c);
    void point(const glm::vec3& p, const glm::vec4& c = basic_colors::kWhite);
    void ray(const glm::vec3& o, const glm::vec3& dir, float len, const glm::vec4& c = basic_colors::kYellow);
    void arrow(const glm::vec3& a, const glm::vec3& b, const glm::vec4& c = basic_colors::kYellow);
    void mesh(resource_handle h, const glm::mat4& model, const glm::vec4& tint = basic_colors::kWhite, bool wireframe = false);

   private:
    render_stream* stream;
  };

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_SCENE_DRAW_HPP