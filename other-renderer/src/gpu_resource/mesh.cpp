/**
 * \file gpu_resource/mesh.cpp
 **/
#include "gpu_resource/mesh.hpp"

#include "core/defines.hpp"
#include "core/logger.hpp"

#include "model/vertex.hpp"
#include "renderer/renderer_backend.hpp"

namespace other {
  namespace {

    value_type get_value_type(mesh::attribute_type type, size_t size) {
      switch (type) {
        case mesh::attribute_type::BYTE:
          return value_type::INT8;
          break;
        case mesh::attribute_type::UNSIGNED_BYTE:
          return value_type::UINT8;
          break;
        case mesh::attribute_type::SHORT:
          return value_type::INT16;
          break;
        case mesh::attribute_type::UNSIGNED_SHORT:
          return value_type::UINT16;
          break;
        case mesh::attribute_type::INT:
          return value_type::INT32;
          break;
        case mesh::attribute_type::UNSIGNED_INT:
          return value_type::UINT32;
          break;

        case mesh::attribute_type::FLOAT:
        case mesh::attribute_type::DOUBLE:
          switch (size) {
            case 1:
              return value_type::FLOAT;
              break;
            case 2:
              return value_type::VEC2;
              break;
            case 3:
              return value_type::VEC3;
              break;
            case 4:
              return value_type::VEC4;
              break;
            default:
              OTHER_ASSERT(false, "Invalid size for FLOAT attribute: {}", size);
          }
          break;
        default:
          OTHER_ASSERT(false, "Unimplemented attribute type: {}", type);
      }
    }

  }  // namespace

  mesh& mesh::bind() {
    subsystem<renderer_backend>::get()->api()->bind_mesh_resource(handle());
    return *this;
  }

  mesh& mesh::upload_vertex_buffer(const std::string_view res_name, gpu_buffer::usage usage, uint32_t vertex_count, const void* data, size_t size) {
    destroy_vertex_buffer();

    bind();
    vertex_buffer_handle = subsystem<renderer_backend>::get()->api()->create_resource(std::string{ res_name }, resource_type::BUFFER);
    if (vertex_buffer_handle.id == 0) {
      CORE_LOG_ERROR("Failed to create vertex buffer resource for mesh.");
      return *this;
    }

    gpu_buffer* vert_buffer = subsystem<renderer_backend>::get()->api()->get_resource_as<gpu_buffer>(vertex_buffer_handle);
    vert_buffer->set_buffer_type(gpu_buffer::buf_type::VERTEX_BUFFER)
      .set_usage(usage)
      .set_data(data, size);
    unbind();

    vert_count = vertex_count;

    return *this;
  }

  mesh& mesh::upload_index_buffer(const std::string_view res_name, gpu_buffer::usage usage, uint32_t index_count, const void* data, size_t size) {
    destroy_index_buffer();

    bind();
    index_buffer_handle = subsystem<renderer_backend>::get()->api()->create_resource(std::string{ res_name }, resource_type::BUFFER);
    if (index_buffer_handle->id == 0) {
      CORE_LOG_ERROR("Failed to create index buffer resource for mesh.");
      return *this;
    }

    gpu_buffer* index_buffer = subsystem<renderer_backend>::get()->api()->get_resource_as<gpu_buffer>(*index_buffer_handle);
    index_buffer->set_buffer_type(gpu_buffer::buf_type::INDEX_BUFFER)
      .set_usage(usage)
      .set_data(data, size);
    unbind();

    this->index_count = index_count;
    return *this;
  }

  mesh& mesh::set_primitive_type(primitive_type type) {
    if (type >= primitive_type::NUM_PRIMITIVE_TYPES) {
      CORE_LOG_ERROR("Invalid primitive type: out of range.");
      return *this;
    }
    prim_type = type;
    return *this;
  }

  mesh& mesh::set_vertex_count(size_t count) {
    vert_count = count;
    return *this;
  }

  mesh& mesh::set_index_count(size_t count) {
    index_count = count;
    return *this;
  }

  mesh& mesh::add_attribute(const std::string& name, mesh::attribute_type type, size_t size, size_t offset) {
    if (type >= attribute_type::NUM_ATTRIBUTE_TYPES) {
      CORE_LOG_ERROR("Invalid attribute type: out of range.");
      return *this;
    }
    if (size == 0 || offset < 0) {
      CORE_LOG_ERROR("Invalid size of offset : size = {} \\ offset = {}.", size, offset);
      return *this;
    }

    vertex_attribute attr;
    value_type vtype = get_value_type(type, size);
    add_attribute(name, vtype, size, offset);
    return *this;
  }

  mesh& mesh::add_attribute(const std::string& name, value_type type, size_t size, size_t offset) {
    size_t idx = attributes.size();
    CORE_LOG_DEBUG("Adding attribute '{}' of type {} at index {}, size {}, offset {} to mesh with handle {}", name, static_cast<uint8_t>(type), idx, size, offset, handle().id);

    vertex_attribute& vattr = attributes.emplace_back();
    vattr.idx = idx;
    vattr.name = name;
    vattr.type = type;
    vattr.size = size;
    vattr.offset = offset;

    return *this;
  }

  mesh& mesh::add_attribute(vertex_attribute attr) {
    if (attr.size == 0 || attr.offset < 0) {
      CORE_LOG_ERROR("Invalid size of offset : size = {} \\ offset = {}.", attr.size, attr.offset);
      return *this;
    }

    size_t idx = attributes.size();
    CORE_LOG_DEBUG("Adding attribute '{}' of type {} at index {}, size {}, offset {} to mesh with handle {}", attr.name, attr.type, idx, attr.size, attr.offset, handle().id);

    attributes.push_back(attr);
    return *this;
  }

  void mesh::draw() {
    if (vertex_buffer_handle.id == 0) {
      CORE_LOG_ERROR("Vertex buffer not uploaded, cannot draw mesh.");
      return;
    }

    if (index_buffer_handle.has_value() && index_buffer_handle->id != 0) {
      subsystem<renderer_backend>::get()->api()->draw_mesh(handle(), prim_type, vert_count, index_count, mesh::attribute_type::UNSIGNED_BYTE);
    } else {
      subsystem<renderer_backend>::get()->api()->draw_mesh(handle(), prim_type, vert_count);
    }
  }

  void mesh::unbind() {
    if (vertex_buffer_handle.id == 0) {
      CORE_LOG_ERROR("Vertex buffer not uploaded, cannot unbind mesh.");
      return;
    }
    subsystem<renderer_backend>::get()->api()->unbind_mesh_resource(handle());
  }

  void mesh::finalize_mesh() {
    if (vertex_buffer_handle.id == 0) {
      CORE_LOG_ERROR("Vertex buffer not uploaded, cannot finalize mesh.");
      return;
    }

    bind();
    gpu_buffer* vert_buffer = subsystem<renderer_backend>::get()->api()->get_resource_as<gpu_buffer>(vertex_buffer_handle);
    vert_buffer->finalize_buffer();
    vert_buffer->bind();

    gpu_buffer* index_buffer = nullptr;
    if (index_buffer_handle.has_value() && index_buffer_handle->id != 0) {
      index_buffer = subsystem<renderer_backend>::get()->api()->get_resource_as<gpu_buffer>(*index_buffer_handle);
      index_buffer->finalize_buffer();
      index_buffer->bind();
    }

    subsystem<renderer_backend>::get()->api()->set_mesh_vertex_attributes(handle(), attributes);

    if (index_buffer != nullptr) {
      index_buffer->unbind();
    }
    vert_buffer->unbind();
    unbind();
  }

  void mesh::destroy_resources() {
    destroy_vertex_buffer();
    destroy_index_buffer();
  }

  void mesh::destroy_vertex_buffer() {
    if (vertex_buffer_handle.id != 0) {
      subsystem<renderer_backend>::get()->api()->destroy_resource(vertex_buffer_handle);
      vertex_buffer_handle = { 0, resource_type::EMPTY };
    }
  }

  void mesh::destroy_index_buffer() {
    if (index_buffer_handle.has_value() && index_buffer_handle->id != 0) {
      subsystem<renderer_backend>::get()->api()->destroy_resource(*index_buffer_handle);
      index_buffer_handle.reset();
    }
  }

}  // namespace other