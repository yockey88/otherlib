/**
 * \file renderer/renderer_backend.cpp
 **/
#include "renderer/renderer_backend.hpp"

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_video.h"

#undef main
#include <SDL3/SDL.h>
#include <imgui/imgui.h>

#include "core/fnv.hpp"
#include "core/logger.hpp"

#include "renderer/backends/opengl_api.hpp"

namespace other {
  namespace backend_keys {

    static constexpr std::string_view kOpenGL = "opengl";
    static constexpr std::string_view kVulkan = "vulkan";
    static constexpr std::string_view kDirectX = "directx";
    static constexpr std::string_view kMetal = "metal";
    static constexpr std::string_view kSoftware = "software";
    static constexpr std::string_view kNull = "null";

    static constexpr natural_t kOpenGLHash = FNV(kOpenGL);
    static constexpr natural_t kVulkanHash = FNV(kVulkan);
    static constexpr natural_t kDirectXHash = FNV(kDirectX);
    static constexpr natural_t kMetalHash = FNV(kMetal);
    static constexpr natural_t kSoftwareHash = FNV(kSoftware);
    static constexpr natural_t kNullHash = FNV(kNull);

  }  // namespace backend_keys

  void renderer_backend::load_backend(const std::string& name, const glm::uvec2& window_size) {
    /// load sdl3
    if (!SDL_Init(SDL_INIT_VIDEO)) {
      CORE_LOG_ERROR("Failed to initialize SDL: {}", SDL_GetError());
      return;
    }

    uint32_t flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;

    natural_t hash = FNV(name);
    switch (hash) {
      case backend_keys::kOpenGLHash: {
        flags |= SDL_WINDOW_OPENGL;
      } break;

      default:
        CORE_LOG_ERROR("Unknown/Unimplmented rendering backend: {}", name);
        break;
    }

    {
      scope<window_manager> window_mgr = make_scope<window_manager>();
      CORE_LOG_DEBUG("Creating main window with size: {}x{}", window_size.x, window_size.y);
      SDL_Window* window = window_mgr->create_window("Other Environment", window_size.x, window_size.y, flags);
      OTHER_ASSERT(window != nullptr, "Failed to create main window: {}", SDL_GetError());

      switch (hash) {
        case backend_keys::kOpenGLHash: {
          set_rendering_api(make_scope<opengl_api>(), std::move(window_mgr));
        } break;

        default:
          CORE_LOG_ERROR("Unknown/Unimplmented rendering backend: {}", name);
          break;
      }
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ui_context = ImGui::GetCurrentContext();
    api()->initialize_ui_context();

    state_flags.full_initialization = true;
  }

  void renderer_backend::force_set_backend(scope<rendering_api> api) {
    scope<window_manager> window_mgr = make_scope<window_manager>();
    set_rendering_api(std::move(api), std::move(window_mgr));
    state_flags.forced_api_set = true;
  }

  void renderer_backend::unload_backend() {
    if (rendering_api_instance != nullptr) {
      CORE_LOG_DEBUG("Shutting down rendering API instance.");

      bool should_shutdown_imgui = !state_flags.forced_api_set;
      rendering_api_instance->shutdown_ui_context();

      if (should_shutdown_imgui) {
        ImGui::DestroyContext();
        ui_context = nullptr;
      }

      rendering_api_instance->destroy_windows();

      rendering_api_instance->shutdown();
      rendering_api_instance = nullptr;

      SDL_Quit();
    }

    state_flags.full_initialization = false;
    state_flags.forced_api_set = false;
  }

  void renderer_backend::handle_event(SDL_Event* event) {
    api()->handle_event(event);
  }

  void renderer_backend::add_model_source(natural_t handle, ref<model_source> source) {
    OTHER_ASSERT(source != nullptr, "Model source cannot be null.");
    OTHER_ASSERT(model_sources.find(handle) == model_sources.end(), "Model source with handle {} already exists.", handle);

    model_sources[handle] = std::move(source);
    CORE_LOG_DEBUG("Added model source with handle: {}", handle);
  }

  ref<model_source> renderer_backend::get_model_source(natural_t handle) const {
    auto it = model_sources.find(handle);
    if (it != model_sources.end()) {
      return it->second;
    }

    CORE_LOG_ERROR("Model source with handle {} not found.", handle);
    return nullptr;
  }

  void renderer_backend::remove_model_source(natural_t handle) {
    auto it = model_sources.find(handle);
    if (it != model_sources.end()) {
      model_sources.erase(it);
      CORE_LOG_DEBUG("Removed model source with handle: {}", handle);
    } else {
      CORE_LOG_ERROR("Model source with handle {} not found.", handle);
    }
  }

  void renderer_backend::set_rendering_api(scope<rendering_api> api, scope<window_manager> window_mgr) {
    OTHER_ASSERT(api != nullptr, "Rendering API instance cannot be null.");
    OTHER_ASSERT(window_mgr != nullptr, "Window manager instance cannot be null.");

    rendering_api_instance = std::move(api);
    rendering_api_instance->initialize(std::move(window_mgr));
  }

}  // namespace other