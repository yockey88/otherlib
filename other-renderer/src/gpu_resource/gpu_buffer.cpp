/**
 * \file gpu_resource/gpu_buffer.cpp
 **/
#include "gpu_resource/gpu_buffer.hpp"

#include "core/logger.hpp"

#include "gpu_resource/renderer_resource.hpp"
#include "renderer/renderer_backend.hpp"

namespace other {

  resource_handle gpu_buffer::create(const std::string_view name, buf_type type, usage buf_usage) {
    if (name.empty()) {
      CORE_LOG_ERROR("Buffer name cannot be empty.");
      return { 0, resource_type::EMPTY };
    }
    if (type >= buf_type::NUM_BUFFER_TYPES || buf_usage >= usage::NUM_USAGE_TYPES) {
      CORE_LOG_ERROR("Invalid buffer type or usage: out of range.");
      return { 0, resource_type::EMPTY };
    }

    resource_handle handle = subsystem<renderer_backend>::get()->api()->create_resource(std::string{ name }, resource_type::BUFFER);
    if (handle.id == 0) {
      CORE_LOG_ERROR("Failed to create GPU buffer resource with name: {}", name);
      return { 0, resource_type::EMPTY };
    }
    (*subsystem<renderer_backend>::get()->api()->get_resource_as<gpu_buffer>(handle))
      .set_binding_name(std::string{ name })
      .set_buffer_type(type)
      .set_usage(buf_usage);
    return handle;
  }

  gpu_buffer& gpu_buffer::set_shader_resource(const resource_handle& shader_handle) {
    if (shader_handle.id == 0 || shader_handle.type != resource_type::SHADER) {
      CORE_LOG_ERROR("Invalid shader resource handle: ID is zero or type is not SHADER.");
      return *this;
    }
    shader_resource_handle = shader_handle;
    return *this;
  }

  gpu_buffer& gpu_buffer::set_shader_resource(uint32_t point, const resource_handle& shader_handle) {
    return set_binding_point(point)
      .set_shader_resource(shader_handle);
  }

  gpu_buffer& gpu_buffer::set_usage(usage new_usage) {
    if (new_usage >= usage::NUM_USAGE_TYPES) {
      CORE_LOG_ERROR("Invalid buffer usage type: out of range.");
      return *this;
    }
    buf_usage = new_usage;
    return *this;
  }

  gpu_buffer& gpu_buffer::set_buffer_type(buf_type new_type) {
    if (new_type >= buf_type::NUM_BUFFER_TYPES) {
      CORE_LOG_ERROR("Invalid buffer type: out of range.");
      return *this;
    }
    buffer_type = new_type;
    return *this;
  }

  gpu_buffer& gpu_buffer::bind() {
    if (handle().id == 0) {
      CORE_LOG_ERROR("Buffer handle is invalid, cannot bind buffer.");
      return *this;
    }
    subsystem<renderer_backend>::get()->api()->bind_buffer_resource(handle(), buffer_type);
    return *this;
  }

  gpu_buffer& gpu_buffer::set_binding_point(uint32_t point) {
    binding_point = point;
    return *this;
  }

  gpu_buffer& gpu_buffer::set_binding_name(const std::string& name) {
    binding_name = name;
    return *this;
  }

  gpu_buffer& gpu_buffer::set_data(const void* data, size_t size) {
    current_size = size;
    if (current_size == 0) {
      CORE_LOG_ERROR("Invalid buffer data: size is zero.");
      return *this;
    }
    buffer_data.clear();
    buffer_data.resize(current_size);
    if (data != nullptr) {
      std::memcpy(buffer_data.data(), data, current_size);
    }
    return *this;
  }

  gpu_buffer& gpu_buffer::set_buffer_at(size_t offset, const void* data, size_t size) {
    if (offset + size > current_size) {
      CORE_LOG_ERROR("Buffer overflow: trying to write beyond buffer size.");
      return *this;
    }
    std::memcpy(buffer_data.data() + offset, data, size);
    return *this;
  }

  gpu_buffer& gpu_buffer::upload_range(size_t start, size_t size, const void* data) {
    if (start + size > current_size) {
      CORE_LOG_ERROR("Buffer overflow: trying to write beyond buffer size.");
      return *this;
    }
    std::memcpy(buffer_data.data() + start, data, size);
    subsystem<renderer_backend>::get()->api()->buffer_range(handle(), binding_point, start, get_data_size(), get_data());
    return *this;
  }

  gpu_buffer& gpu_buffer::upload_buffer() {
    subsystem<renderer_backend>::get()->api()->buffer_data(handle(), binding_point, get_data(), get_data_size());
    return *this;
  }

  void gpu_buffer::unbind() {
    if (handle().id == 0) {
      CORE_LOG_ERROR("Buffer handle is invalid, cannot unbind buffer.");
      return;
    }
    subsystem<renderer_backend>::get()->api()->unbind_buffer_resource(handle());
  }

  void gpu_buffer::finalize_buffer() {
    if (shader_resource_handle.has_value()) {
      subsystem<renderer_backend>::get()->api()->bind_shader_buffer_resource(handle(), *shader_resource_handle, *binding_name, binding_point, buffer_type, get_data(), get_data_size());
    } else {
      upload_buffer();
    }
  }

  uint8_t* gpu_buffer::get_data() {
    return buffer_data.empty() ? nullptr : buffer_data.data();
  }

  size_t gpu_buffer::get_data_size() {
    return current_size;
  }

}  // namespace other