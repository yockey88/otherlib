/**
 * \file renderer/renderer.cpp
 **/
#include "renderer/renderer.hpp"

#include "renderer/renderer_backend.hpp"

#include "SDL3/SDL.h"
#include "SDL3/SDL_mouse.h"

namespace other {

  void renderer::begin_frame() {
    rendering()->api()->begin_frame();
  }

  void renderer::end_frame() {
    rendering()->api()->end_frame();
  }

  void renderer::begin_ui_frame() {
    rendering()->api()->begin_ui_frame();
  }
  void renderer::end_ui_frame() {
    rendering()->api()->end_ui_frame();
  }

  glm::ivec2 renderer::get_window_size() {
    SDL_Window* window = rendering()->api()->window_handle();
    if (window == nullptr) {
      CORE_LOG_ERROR("SDL window handle is null, cannot get window size.");
      return { 0, 0 };
    }

    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    return { width, height };
  }

  void renderer::set_clear_color(const glm::vec4& color) {
    rendering()->api()->set_clear_color(color);
  }

  glm::vec2 renderer::get_mouse_position() {
    SDL_Window* window = rendering()->api()->window_handle();
    if (window == nullptr) {
      return {};
    }

    float x, y;
    SDL_MouseButtonFlags _ = SDL_GetMouseState(&x, &y);
    return { x, y };
  }

  resource_handle renderer::create_resource(const std::string& name, resource_type type) {
    return rendering()->api()->create_resource(name, type);
  }

  void renderer::destroy_resource(const resource_handle& handle) {
    rendering()->api()->destroy_resource(handle);
  }

  renderer_backend* renderer::rendering() {
    return subsystem<renderer_backend>::get();
  }

}  // namespace other