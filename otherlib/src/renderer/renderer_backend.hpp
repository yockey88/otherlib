/**
 * \file renderer_backend.hpp
 **/
#ifndef OTHERLIB_RENDERER_BACKEND_HPP
#define OTHERLIB_RENDERER_BACKEND_HPP

#include <SDL3/SDL.h>

#include "core/scope.hpp"
#include "core/subsystem.hpp"
#include "renderer/rendering_api.hpp"

namespace other {

  class renderer_backend : public subsystem<renderer_backend> {
   public:
    renderer_backend() = default;

    SDL_Window* get_window() const { return window; }
    scope<rendering_api>& api() { return rendering_api_instance; }
    bool has_backend() const { return rendering_api_instance != nullptr; }

    void load_backend(const std::string& name);
    void unload_backend();

    void handle_event(SDL_Event* event);
    void add_window_listener(SDL_EventFilter callback);

    static void on_set(renderer_backend* instance);

   protected:
    friend class renderer;

    SDL_Window* window = nullptr;
    scope<rendering_api> rendering_api_instance;

    void set_rendering_api(scope<rendering_api> api);
  };

  template <>
  struct subsystem_description<renderer_backend> {
    static constexpr size_t size = sizeof(renderer_backend);
    static constexpr size_t alignment = alignof(renderer_backend);
    static inline subsystem_storage_t<renderer_backend> storage;

    static renderer_backend* ptr() {
      return std::launder(reinterpret_cast<renderer_backend*>(&storage));
    }

    static void* address() {
      return reinterpret_cast<void*>(&storage);
    }
  };

}  // namespace other

#endif  // OTHERLIB_RENDERER_BACKEND_HPP