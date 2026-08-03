/**
 * @file renderer/renderer_backend.hpp
 */
#ifndef OTHER_RENDERER_RENDERER_RENDERER_BACKEND_HPP
#define OTHER_RENDERER_RENDERER_RENDERER_BACKEND_HPP

#include <SDL3/SDL.h>

#include "core/scope.hpp"
#include "core/subsystem.hpp"

#include "gpu_resource/material.hpp"
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

    /// gpu half of a model source; upload interleaves via vertex::to_gpu_buffer into one MESH
    ///  resource. main thread only. remove_model_source and shutdown call destroy_model.
    void upload_model(model_source& source);
    void destroy_model(model_source& source);

    /// texture assets keyed by asset path hash, mirroring model sources; removal
    /// destroys the gpu resource
    void add_texture(natural_t handle, resource_handle texture_handle);
    resource_handle get_texture(natural_t handle) const;
    void remove_texture(natural_t handle);

    /// material assets keyed by asset path hash; pure cpu data (no gpu resource). the
    ///  stored revision is monotonic per handle across remove/add cycles so pipeline pack
    ///  caches invalidate on refresh (the refresh sequence runs the unload half first, which
    ///  is why add still asserts on duplicates as programmer error)
    void add_material(natural_t handle, material mat);
    const material* get_material(natural_t handle) const;
    void remove_material(natural_t handle);

    /// standalone animation clips keyed by asset path hash; pure cpu data, immutable after
    ///  registration — playback state lives with the player (doc 03). same add/get/remove +
    ///  refresh-replace contract as materials; embedded clips stay on their model_source
    void add_animation(natural_t handle, animation_clip clip);
    const animation_clip* get_animation(natural_t handle) const;
    void remove_animation(natural_t handle);

    enum class fallback_texture : uint8_t { WHITE, FLAT_NORMAL };
    /// 1x1 stand-ins bound for material texture slots with no loaded texture — an untextured
    ///  material samples white and multiplies by its params, so there are zero shader
    ///  variants. created lazily on first use, destroyed in unload_backend.
    resource_handle get_fallback_texture(fallback_texture kind);

   protected:
    friend class renderer;

    ImGuiContext* ui_context = nullptr;
    scope<rendering_api> rendering_api_instance;

    ostd::map<natural_t, ref<model_source>> model_sources;
    ostd::map<natural_t, resource_handle> texture_assets;
    ostd::map<natural_t, material> material_assets;
    ostd::map<natural_t, uint32_t> material_revisions;  //< high-water marks, survive remove_material
    ostd::map<natural_t, animation_clip> animation_assets;

    resource_handle fallback_white = {};
    resource_handle fallback_flat_normal = {};

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