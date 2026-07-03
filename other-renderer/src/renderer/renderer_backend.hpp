/**
 * @file renderer/renderer_backend.hpp
 */
#ifndef OTHER_RENDERER_RENDERER_RENDERER_BACKEND_HPP
#define OTHER_RENDERER_RENDERER_RENDERER_BACKEND_HPP

#include <SDL3/SDL.h>

#include "core/scope.hpp"
#include "core/subsystem.hpp"

#include "model/model.hpp"
#include "model/model_source.hpp"
#include "renderer/rendering_api.hpp"

struct ImGuiContext;
namespace other {

  constexpr static ImWchar kUnicodeExtraRanges[] = {
    0x0020, 0xFFFF,  /// enough range to cover all unicode characters
    0
  };

  class renderer_backend : public subsystem<renderer_backend> {
   public:
    renderer_backend() = default;

    static void on_set(renderer_backend* instance);

    SDL_Window* get_main_window() const { return rendering_api_instance->window_handle(); }

    ImGuiContext* get_ui_context() const { return ui_context; }

    scope<rendering_api>& api() { return rendering_api_instance; }
    bool has_backend() const { return rendering_api_instance != nullptr; }

    void load_backend(const config_table& config, const std::string& name, const glm::uvec2& window_size);

    /// don't ever use this unless you really know what you're doing,
    /// it will skip proper initialization steps, useful for testing, etc.
    void force_set_backend(scope<rendering_api> api);
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

    struct {
      bool backend_loaded = false;
      bool ui_initialized = false;
      bool full_initialization = false;
      bool forced_api_set = false;
    } state_flags;

    void set_rendering_api(scope<rendering_api> api, scope<window_manager> window_mgr);
  };

}  // namespace other

OTHER_DEPENDENT_SUBSYSTEM(
  other::renderer_backend,
  subsystem_profile::kArena,
  subsystem_profile::kLogger,
  subsystem_profile::kFileSystem);

#endif  // OTHER_RENDERER_RENDERER_RENDERER_BACKEND_HPP