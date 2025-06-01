/**
 * @file renderer/renderer_backend.hpp
 */
#ifndef OTHER_RENDERER_RENDERER_RENDERER_BACKEND_HPP
#define OTHER_RENDERER_RENDERER_RENDERER_BACKEND_HPP

#include <SDL3/SDL.h>

#include "core/scope.hpp"
#include "core/subsystem.hpp"
#include "renderer/rendering_api.hpp"

namespace other {

  class renderer_backend : public subsystem<renderer_backend> {
   public:
    renderer_backend() = default;

    SDL_Window* get_main_window() const { return rendering_api_instance->window_handle(); }
    scope<rendering_api>& api() { return rendering_api_instance; }
    bool has_backend() const { return rendering_api_instance != nullptr; }

    void load_backend(const std::string& name);
    void unload_backend();

    void handle_event(SDL_Event* event);

    static void on_set(renderer_backend* instance);

   protected:
    friend class renderer;

    scope<rendering_api> rendering_api_instance;

    void set_rendering_api(scope<rendering_api> api, scope<window_manager> window_mgr);
  };

  OTHER_SUBSYSTEM(renderer_backend);

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_RENDERER_BACKEND_HPP