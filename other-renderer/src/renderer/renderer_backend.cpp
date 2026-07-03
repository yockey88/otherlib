/**
 * \file renderer/renderer_backend.cpp
 **/
#include "renderer/renderer_backend.hpp"

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_video.h"

#undef main
#include <SDL3/SDL.h>
#include <imgui/imgui.h>

#include "core/config_table.hpp"
#include "core/fnv.hpp"
#include "core/logger.hpp"
#include "core/profiler.hpp"

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

  namespace detail {

    /// imgui functions so that imgui works across dll boundaries

    // typedef void* (*ImGuiMemAllocFunc)(size_t sz, void* user_data);  // Function signature for ImGui::SetAllocatorFunctions()
    // typedef void (*ImGuiMemFreeFunc)(void* ptr, void* user_data);    // Function signature for ImGui::SetAllocatorFunctions()

    void* imgui_allocate(size_t size, void* user_data) {
      return arena::allocate(size);
    }

    void imgui_deallocate(void* ptr, void* user_data) {
      arena::free(ptr);
    }

  };  // namespace detail

  void renderer_backend::on_set(renderer_backend* instance) {
    GImGui = instance->ui_context;
  }

  void renderer_backend::load_backend(const config_table& config, const std::string& name, const glm::uvec2& window_size) {
    PROFILE_SECTION("renderer_backend::load-backend");
    uint32_t flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;

    {
      PROFILE_SECTION("renderer_backend::load-backend--initialize-sdl3");

      if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        CORE_LOG_ERROR("Failed to initialize SDL: {}", SDL_GetError());
        return;
      }
    }

    {
      PROFILE_SECTION("renderer_backend::load-backend--create-window-and-load-api");

      scope<window_manager> window_mgr = make_scope<window_manager>();
      CORE_LOG_DEBUG("Creating main window with size: {}x{}", window_size.x, window_size.y);

      natural_t hash = FNV(name);
      switch (hash) {
        case backend_keys::kOpenGLHash: flags |= SDL_WINDOW_OPENGL; break;
        default:
          CORE_LOG_ERROR("Unknown/Unimplmented rendering backend: {}", name);
          break;
      }

      SDL_Window* window = window_mgr->create_window("Other Environment", window_size.x, window_size.y, flags);
      OTHER_ASSERT(window != nullptr, "Failed to create main window: {}", SDL_GetError());

      switch (hash) {
        case backend_keys::kOpenGLHash: set_rendering_api(make_scope<opengl_api>(), std::move(window_mgr)); break;
        default:
          CORE_LOG_ERROR("Unknown/Unimplmented rendering backend: {}", name);
          break;
      }

      state_flags.backend_loaded = true;
    }

    std::string ui_ini_name = "resources/ui/default_ui_layout.ini";
    static std::string real_ini_name = config.get_value<std::string>("rendering.ui-layout-ini", ui_ini_name);
    static std::string main_imgui_font = config.get_value<std::string>("rendering.imgui-font", "${other-directory}/resources/fonts/BlexMonoNerdFont-Regular.ttf");
    {
      PROFILE_SECTION("renderer_backend::load-backend--imgui-init");
      IMGUI_CHECKVERSION();
      ImGui::CreateContext();
      ImGuiIO& io = ImGui::GetIO();
      io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
      io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
      io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
      io.ConfigWindowsMoveFromTitleBarOnly = true;
      io.IniFilename = real_ini_name.c_str();
      ImGui::StyleColorsDark();
      OTHER_ASSERT(std::filesystem::exists(main_imgui_font), "Failed to find ImGui font: {}", main_imgui_font);
      ImFontConfig config;

      ImFont* font = ImGui::GetIO().Fonts->AddFontFromFileTTF(main_imgui_font.c_str(), 16.0f, &config, kUnicodeExtraRanges);
      OTHER_ASSERT(font != nullptr, "Failed to load ImGui font: {}", main_imgui_font);
      io.FontDefault = font;

      ui_context = ImGui::GetCurrentContext();
      api()->initialize_ui_context();

      ImGui::SetAllocatorFunctions(&detail::imgui_allocate, &detail::imgui_deallocate);

      ui_context = GImGui;
    }

    state_flags.full_initialization = true;
  }

  void renderer_backend::force_set_backend(scope<rendering_api> api) {
    scope<window_manager> window_mgr = make_scope<window_manager>();
    set_rendering_api(std::move(api), std::move(window_mgr));
    state_flags.forced_api_set = true;
  }

  void renderer_backend::unload_backend() {
    PROFILE_SECTION("renderer_backend::unload_backend");
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
      rendering_api_instance->verify_shutdown();
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

    return nullptr;
  }

  void renderer_backend::remove_model_source(natural_t handle) {
    auto it = model_sources.find(handle);
    if (it != model_sources.end()) {
      it->second->destroy_resources();

      model_sources.erase(it);
      CORE_LOG_DEBUG("Removed model source with handle: {}", handle);
    } else {
      CORE_LOG_ERROR("Model source with handle {} not found.", handle);
    }
  }

  void renderer_backend::set_rendering_api(scope<rendering_api> api, scope<window_manager> window_mgr) {
    OTHER_ASSERT(api != nullptr, "Rendering API instance cannot be null.");
    OTHER_ASSERT(window_mgr != nullptr, "Window manager instance cannot be null.");
    PROFILE_SECTION("renderer_backend::set-rendering-api");
    rendering_api_instance = std::move(api);
    rendering_api_instance->initialize(std::move(window_mgr));
  }

}  // namespace other