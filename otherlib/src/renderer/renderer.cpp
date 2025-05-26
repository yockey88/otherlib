/**
 * \file renderer/renderer.cpp
 **/
#include "renderer/renderer.hpp"

#include "core/logger.hpp"
#include "renderer/renderer_backend.hpp"

namespace other {

  void renderer::begin_frame() {
    rendering()->rendering_api_instance->begin_frame();
  }

  void renderer::end_frame() {
    rendering()->rendering_api_instance->end_frame();
  }

  void renderer::set_clear_color(const glm::vec4& color) {
    auto* r = rendering();
    r->rendering_api_instance->set_clear_color(color);
  }

  resource_handle renderer::create_resource(resource_type type) {
    return rendering()->rendering_api_instance->create_resource(type);
  }

  renderer_backend* renderer::rendering() {
    return subsystem<renderer_backend>::get();
  }

}  // namespace other