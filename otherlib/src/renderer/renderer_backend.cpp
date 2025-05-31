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

  void renderer_backend::load_backend(const std::string& name) {
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

    window = SDL_CreateWindow("Other Environment", 1280, 720, flags);
    if (window == nullptr) {
      CORE_LOG_ERROR("Failed to create SDL window: {}", SDL_GetError());
      return;
    }

    switch (hash) {
      case backend_keys::kOpenGLHash: {
        set_rendering_api(make_scope<opengl_api>(window));
      } break;

      default:
        CORE_LOG_ERROR("Unknown/Unimplmented rendering backend: {}", name);
        break;
    }

    rendering_api_instance->initialize();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = "./config/imgui.ini";
    ImGui::StyleColorsDark();

    rendering_api_instance->initialize_ui_context();
  }

  void renderer_backend::unload_backend() {
    if (rendering_api_instance != nullptr) {
      CORE_LOG_DEBUG("Shutting down rendering API instance.");
      rendering_api_instance->shutdown_ui_context();
      ImGui::DestroyContext();

      rendering_api_instance->shutdown();
      rendering_api_instance = nullptr;

      if (window != nullptr) {
        SDL_DestroyWindow(window);
        window = nullptr;
      }
      SDL_Quit();
    }
  }

  void renderer_backend::handle_event(SDL_Event* event) {
    rendering_api_instance->handle_event(event);
  }

  void renderer_backend::add_window_listener(SDL_EventFilter callback) {
    SDL_AddEventWatch(callback, window);
  }

  void renderer_backend::on_set(renderer_backend* instance) {
    /// set imgui context on this side of the dll boundary
    // ImGui::SetCurrentContext((ImGuiContext*)instance->rendering_api_instance->get_context_handle());
  }

  void renderer_backend::set_rendering_api(other::scope<rendering_api> api) {
    rendering_api_instance = std::move(api);
  }

}  // namespace other