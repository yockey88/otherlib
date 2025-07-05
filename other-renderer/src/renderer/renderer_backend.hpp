/**
 * @file renderer/renderer_backend.hpp
 */
#ifndef OTHER_RENDERER_RENDERER_RENDERER_BACKEND_HPP
#define OTHER_RENDERER_RENDERER_RENDERER_BACKEND_HPP

#include <SDL3/SDL.h>

#include "core/scope.hpp"
#include "core/subsystem.hpp"

#include "model/model.hpp"
#include "renderer/rendering_api.hpp"

struct ImGuiContext;
namespace other {

  class renderer_backend : public subsystem<renderer_backend> {
   public:
    renderer_backend() = default;

    SDL_Window* get_main_window() const { return rendering_api_instance->window_handle(); }

    ImGuiContext* get_ui_context() const { return ui_context; }

    scope<rendering_api>& api() { return rendering_api_instance; }
    bool has_backend() const { return rendering_api_instance != nullptr; }

    void load_backend(const std::string& name);
    void unload_backend();

    void handle_event(SDL_Event* event);

    void add_model_source(natural_t handle, ref<model_source> source);
    ref<model_source> get_model_source(natural_t handle) const;
    void remove_model_source(natural_t handle);

   protected:
    friend class renderer;

    ImGuiContext* ui_context = nullptr;
    scope<rendering_api> rendering_api_instance;

    std::map<natural_t, ref<model_source>> model_sources;

    void set_rendering_api(scope<rendering_api> api, scope<window_manager> window_mgr);
  };

  OTHER_SUBSYSTEM(renderer_backend);

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_RENDERER_BACKEND_HPP