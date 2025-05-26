/**
 * \file renderer/rendering_api.cpp
 **/
#include "renderer/rendering_api.hpp"

#include "core/logger.hpp"

namespace other {

  resource_handle rendering_api::create_resource(resource_type type) {
    resource_handle handle = { get_next_resource_id(), type };

    void* resource = nullptr;
    switch (type) {
      case resource_type::BUFFER:
        resource = create_buffer_resource(handle.id, type);
        break;

      case resource_type::TEXTURE:
        resource = create_texture_resource(handle.id, type);
        break;

      case resource_type::SHADER:
        resource = create_shader_resource(handle.id, type);
        break;

      default: break;
    }
    if (resource == nullptr) {
      CORE_LOG_ERROR("Failed to create resource of type: {}", type);
      return { 0, resource_type::EMPTY };
    }

    resource_handles[handle.id] = handle;
    resources[handle.id] = get_resource(handle.id);
  }

  void rendering_api::set_resource_name(const resource_handle& handle, const std::string& name) {
    auto itr = std::ranges::find_if(resource_names, [&](const auto& pair) {
      return pair.second.name == name;
    });
    if (itr != resource_names.end()) {
      CORE_LOG_ERROR("Resource with name '{}' already exists.", name);
      return;
    }

    resource_names[handle.id] = { name, FNV(name) };
  }

  bool rendering_api::resource_has_name(const resource_handle& handle, uint64_t name_hash) const {
    auto itr = resource_names.find(handle.id);
    if (itr != resource_names.end()) {
      return itr->second.hash == name_hash;
    }
    return false;
  }

}  // namespace other