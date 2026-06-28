/**
 * \file renderer/rendering_api.cpp
 **/
#include "renderer/rendering_api.hpp"

#include <imgui/backends/imgui_impl_sdl3.h>
#include <imguizmo/ImGuizmo.h>

#include "core/defines.hpp"
#include "core/enum_formatter.hpp"
#include "core/fnv.hpp"
#include "core/logger.hpp"

namespace other {

  SDL_Window* rendering_api::window_handle() {
    return native_window();
  }

  void* rendering_api::get_context_handle() {
    return window_mgr->get_main_window();
  }

  void rendering_api::initialize(scope<window_manager> window_mgr) {
    PROFILE_SECTION("rendering_api::initialize");
    this->window_mgr = std::move(window_mgr);
    {
      PROFILE_SECTION("rendering_api::initialize--client-on_initialize");
      on_initialize(this->window_mgr);
    }
  }

  void rendering_api::shutdown() {
    on_shutdown(this->window_mgr);
    window_mgr = nullptr;
  }

  uint32_t rendering_api::full_mip_chain_count(const glm::ivec2& size, texture::tex_type type, uint32_t depth) const {
    uint32_t m = (uint32_t)std::max(size.x, size.y);
    if (type == texture::tex_type::TEXTURE_3D) {
      m = std::max(m, depth);
    }

    uint32_t levels = 1;
    while (m > 1) {
      m >>= 1;
      ++levels;
    }
    return levels;  // 1 + floor(log2(max_dim))
  }

  void rendering_api::destroy_windows() {
    if (native_window() != nullptr) {
      SDL_DestroyWindow(native_window());
    }
  }

  void rendering_api::begin_frame() {
    on_begin_frame(window_mgr);
  }

  void rendering_api::end_frame() {
    on_end_frame(window_mgr);
  }

  void rendering_api::begin_ui_frame() {
    if (get_gpu_context() == nullptr) {
      CORE_LOG_ERROR("Rendering API context handle is null, cannot begin UI frame.");
      return;
    }

    begin_ui_frame_backend_newframe();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
  }

  void rendering_api::end_ui_frame() {
    if (get_gpu_context() == nullptr) {
      CORE_LOG_ERROR("Rendering API context handle is null, cannot end UI frame.");
      return;
    }

    ImGui::Render();
    end_ui_frame_backend_draw_data();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
  }

  resource_handle rendering_api::create_resource(const std::string_view name, resource_type type) {
    resource_handle handle = { get_next_resource_id(), type };
    handle.name_hash = FNV(name);
    CORE_LOG_DEBUG("Creating GPU resource [{}] : {}", name, handle);

    resource* res = nullptr;
    switch (type) {
      case resource_type::SHADER:
        res = create_shader_resource(handle, type);
        break;

      case resource_type::TEXTURE:
        res = create_texture_resource(handle, type);
        break;

      case resource_type::CUBEMAP:
        res = create_cube_map_resource(handle, type);
        break;

      case resource_type::BUFFER:
        res = create_buffer_resource(handle, type);
        break;

      case resource_type::MESH:
        res = create_mesh_resource(handle, type);
        break;

      case resource_type::FRAMEBUFFER:
        res = create_framebuffer_resource(handle, type);
        break;

      default:
        CORE_LOG_ERROR("Unsupported resource type: {}", type);
        return { 0, resource_type::EMPTY };
    }

    if (res == nullptr) {
      CORE_LOG_ERROR("Failed to create resource of type: {}", type);
      return { 0, resource_type::EMPTY };
    }

    res->name = std::string{ name };

    auto [itr, inserted] = resource_handles.emplace(handle.id, handle);
    if (!inserted || itr == resource_handles.end()) {
      CORE_LOG_ERROR("Failed to create resource handle for ID: {}", handle.id);
      return { 0, resource_type::EMPTY };
    }

    auto [itr2, inserted2] = resources.emplace(handle.id, res);
    if (!inserted2 || itr2 == resources.end()) {
      CORE_LOG_ERROR("Failed to create resource for ID: {}", handle.id);
      return { 0, resource_type::EMPTY };
    }

    CORE_LOG_DEBUG("      Created resource of type: {}, ID: {}", type, handle.id);
    set_resource_name(handle, name);
    return handle;
  }

  void rendering_api::destroy_resource(const resource_handle& handle) {
    auto itr = resources.find(handle.id);
    if (itr == resources.end()) {
      CORE_LOG_ERROR("Resource with ID {} not found.", handle.id);
      return;
    }

    resources.erase(itr);
    {
      auto itr = resource_names.find(handle.id);
      if (itr != resource_names.end()) {
        resource_names.erase(itr);
      } else {
        CORE_LOG_ERROR("Resource name for ID {} not found.", handle.id);
      }
    }
    CORE_LOG_DEBUG("Destroying resource [{}]", handle);

    switch (handle.type) {
      case resource_type::SHADER:
        destroy_shader_resource(handle);
        break;

      case resource_type::TEXTURE:
        destroy_texture_resource(handle);
        break;

      case resource_type::CUBEMAP:
        destroy_cube_map_resource(handle);
        break;

      case resource_type::BUFFER:
        destroy_buffer_resource(handle);
        break;

      case resource_type::MESH:
        destroy_mesh_resource(handle);
        break;

      case resource_type::FRAMEBUFFER:
        destroy_framebuffer_resource(handle);
        break;

      default:
        CORE_LOG_ERROR("Unsupported resource type for destruction: {}", handle.type);
        break;
    }
  }

  void rendering_api::set_resource_name(const resource_handle& handle, const std::string_view name) {
    auto [name_itr, inserted] = resource_names.emplace(handle.id, name);
    if (!inserted || name_itr == resource_names.end()) {
      CORE_LOG_ERROR("Failed to set name '{}' for resource ID: {}", name, handle.id);
      return;
    }
  }

  bool rendering_api::resource_exists(const resource_handle& handle) {
    auto itr = resources.find(handle.id);
    return itr != resources.end();
  }

  resource* rendering_api::get_resource(natural_t id) {
    auto itr = resources.find(id);
    if (itr != resources.end()) {
      return itr->second;
    }

    CORE_LOG_ERROR("Resource with ID {} not found.", id);
    return nullptr;
  }

  std::string rendering_api::get_resource_name(const resource_handle& handle) const {
    auto itr = resource_names.find(handle.id);
    if (itr != resource_names.end()) {
      return itr->second;
    }

    CORE_LOG_ERROR("Resource name for ID {} not found.", handle.id);
    return "<unknown>";
  }

  SDL_Window* rendering_api::native_window() {
    return window_mgr->get_main_window();
  }

  void rendering_api::set_gpu_context(void* context) {
    gpu_context = context;
  }

  void* rendering_api::get_gpu_context() const {
    return gpu_context;
  }

  glm::vec3 rendering_api::get_clear_color() const {
    return clear_color;
  }

  void rendering_api::override_clear_color(const glm::vec3& color) {
    clear_color = color;
  }

  void rendering_api::override_clear_depth(float depth) {
    clear_depth = depth;
  }

  void rendering_api::override_clear_stencil(uint32_t stencil) {
    clear_stencil = stencil;
  }

  glm::ivec2 rendering_api::get_window_size() const {
    return window_size;
  }
  void rendering_api::set_window_size(const glm::ivec2& size) {
    window_size = size;
    SDL_SetWindowSize(window_mgr->get_main_window(), size.x, size.y);
  }

}  // namespace other