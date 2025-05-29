/**
 * \file renderer/rendering_api.cpp
 **/
#include "renderer/rendering_api.hpp"

#include <imgui/backends/imgui_impl_sdl3.h>

#include "core/logger.hpp"

namespace other {

  void rendering_api::begin_ui_frame() {
    if (get_gpu_context() == nullptr) {
      CORE_LOG_ERROR("Rendering API context handle is null, cannot begin UI frame.");
      return;
    }

    begin_ui_frame_backend_newframe();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
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

  resource_handle rendering_api::create_resource(const std::string& name, resource_type type) {
    resource_handle handle = { get_next_resource_id(), type };
    handle.name_hash = FNV(name);

    resource* res = nullptr;
    switch (type) {
      case resource_type::SHADER:
        res = create_shader_resource(handle, type);
        break;

      case resource_type::TEXTURE:
        res = create_texture_resource(handle, type);
        break;

      case resource_type::BUFFER:
        res = create_buffer_resource(handle, type);
        break;

      case resource_type::MESH:
        res = create_mesh_resource(handle, type);
        break;

      default:
        CORE_LOG_ERROR("Unsupported resource type: {}", type);
        return { 0, resource_type::EMPTY };
    }

    if (res == nullptr) {
      CORE_LOG_ERROR("Failed to create resource of type: {}", type);
      return { 0, resource_type::EMPTY };
    }

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

    CORE_LOG_DEBUG("Created resource of type: {}, ID: {}", type, handle.id);
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

    switch (handle.type) {
      case resource_type::SHADER:
        destroy_shader_resource(handle);
        break;

      case resource_type::TEXTURE:
        destroy_texture_resource(handle);
        break;

      case resource_type::BUFFER:
        destroy_buffer_resource(handle);
        break;

      case resource_type::MESH:
        destroy_mesh_resource(handle);
        break;

      default:
        CORE_LOG_ERROR("Unsupported resource type for destruction: {}", handle.type);
        break;
    }
  }

  void rendering_api::set_resource_name(const resource_handle& handle, const std::string& name) {
    auto itr = std::ranges::find_if(resource_names, [&](const auto& pair) {
      return pair.second == name;
    });
    if (itr != resource_names.end()) {
      CORE_LOG_ERROR("Resource with name '{}' already exists.", name);
      return;
    }

    auto [name_itr, inserted] = resource_names.emplace(handle.id, name);
    if (!inserted || name_itr == resource_names.end()) {
      CORE_LOG_ERROR("Failed to set name '{}' for resource ID: {}", name, handle.id);
      return;
    }
  }

  resource* rendering_api::get_resource(uint64_t id) {
    auto itr = resources.find(id);
    if (itr != resources.end()) {
      return itr->second;
    }

    CORE_LOG_ERROR("Resource with ID {} not found.", id);
    return nullptr;
  }

}  // namespace other