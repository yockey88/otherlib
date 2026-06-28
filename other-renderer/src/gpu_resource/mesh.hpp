/**
 * \file renderer/mesh.hpp
 **/
#ifndef OTHER_RENDERER_GPU_RESOURCE_MESH_HPP
#define OTHER_RENDERER_GPU_RESOURCE_MESH_HPP

#include "gpu_resource/gpu_buffer.hpp"
#include "gpu_resource/renderer_resource.hpp"
#include "model/vertex.hpp"

namespace other {

  struct mesh : public resource {
    // OTHER_REFLECTABLE(mesh);
    enum primitive_type {
      POINTS = 0,
      LINES,
      LINE_STRIP,
      TRIANGLES,
      TRIANGLE_STRIP,
      TRIANGLE_FAN,

      /// add more here...

      NUM_PRIMITIVE_TYPES
    };
    enum attribute_type {
      INVALID = 0,
      BYTE,
      UNSIGNED_BYTE,
      SHORT,
      UNSIGNED_SHORT,
      INT,
      UNSIGNED_INT,
      FLOAT,
      DOUBLE,

      /// add more here...

      NUM_ATTRIBUTE_TYPES
    };
    struct attribute {
      std::string name;
      attribute_type type = attribute_type::INVALID;
      size_t idx = 0;  // index of the attribute in the vertex buffer
      size_t count = 0;
      size_t offset = 0;
    };

    mesh() = default;
    mesh(resource_handle handle)
        : resource(handle) {}
    virtual ~mesh() = default;

    inline bool has_indices() const { return index_buffer_handle.has_value(); }

    resource_type type() const override { return resource_type::MESH; }

    mesh& bind();
    mesh& upload_vertex_buffer(const std::string_view res_name, gpu_buffer::usage usage, uint32_t vertex_count, const void* data, size_t size);
    mesh& upload_index_buffer(const std::string_view res_name, gpu_buffer::usage usage, uint32_t index_count, const void* data, size_t size);
    mesh& set_primitive_type(primitive_type type);
    mesh& set_vertex_count(size_t count);
    mesh& set_index_count(size_t count);
    mesh& add_attribute(const std::string& name, attribute_type type, size_t size, size_t offset);
    mesh& add_attribute(const std::string& name, value_type type, size_t size, size_t offset);
    mesh& add_attribute(vertex_attribute attr);

    resource_handle vertex_handle() const { return vertex_buffer_handle; }
    opt<resource_handle> index_handle() const { return index_buffer_handle; }

    void draw();

    void unbind();
    void finalize_mesh();

    void destroy_resources();

    size_t vert_count = 0;
    size_t index_count = 0;

   private:
    resource_handle vertex_buffer_handle;
    opt<resource_handle> index_buffer_handle;

    primitive_type prim_type = primitive_type::TRIANGLES;

    std::vector<vertex_attribute> attributes;

    void destroy_vertex_buffer();
    void destroy_index_buffer();
  };

}  // namespace other

#endif  // OTHER_RENDERER_GPU_RESOURCE_MESH_HPP