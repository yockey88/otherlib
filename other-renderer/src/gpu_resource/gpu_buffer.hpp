/**
 * \file gpu_resource/gpu_buffer.hpp
 **/
#ifndef OTHER_RENDERER_GPU_RESOURCE_GPU_BUFFER_HPP
#define OTHER_RENDERER_GPU_RESOURCE_GPU_BUFFER_HPP

#include "gpu_resource/renderer_resource.hpp"

namespace other {

  struct gpu_buffer : public resource {
    // OTHER_REFLECTABLE(gpu_buffer);

    enum usage : uint8_t {
      STATIC = 0,
      DYNAMIC,
      STREAM,

      NUM_USAGE_TYPES
    };
    enum buf_type : uint8_t {
      VERTEX_BUFFER = 0,
      INDEX_BUFFER,

      UNIFORM_BUFFER,
      STORAGE_BUFFER,

      DRAW_INDIRECT_BUFFER,

      /// add more here...

      NUM_BUFFER_TYPES
    };

    gpu_buffer() = default;
    gpu_buffer(resource_handle handle)
        : resource(handle) {}
    virtual ~gpu_buffer() = default;

    resource_type type() const override { return resource_type::BUFFER; }
    usage get_usage() const { return buf_usage; }
    buf_type get_buffer_type() const { return buffer_type; }

    static resource_handle create(const std::string_view name, buf_type type = UNIFORM_BUFFER, usage buf_usage = usage::DYNAMIC);

    gpu_buffer& bind();
    gpu_buffer& set_binding_point(uint32_t point);
    gpu_buffer& set_binding_name(const std::string& name);
    gpu_buffer& set_shader_resource(const resource_handle& shader_handle);
    gpu_buffer& set_shader_resource(uint32_t point, const resource_handle& shader_handle);
    gpu_buffer& set_usage(usage new_usage);
    gpu_buffer& set_buffer_type(buf_type new_type);
    gpu_buffer& set_data(const void* data, size_t size);
    gpu_buffer& set_buffer_at(size_t offset, const void* data, size_t size);
    gpu_buffer& upload_range(size_t start, size_t size, const void* data);
    gpu_buffer& upload_buffer();

    void unbind();
    void finalize_buffer();

   private:
    size_t current_size = 0;
    uint32_t binding_point = 0;
    opt<std::string> binding_name;
    opt<resource_handle> shader_resource_handle;

    usage buf_usage = usage::STATIC;
    buf_type buffer_type = buf_type::UNIFORM_BUFFER;
    std::vector<uint8_t> buffer_data;

    uint8_t* get_data();
    size_t get_data_size();
  };

}  // namespace other

// OTHER_REFLECT(
//   other::gpu_buffer,
//   field(current_size, other::attr::serializable()),
//   field(binding_point, other::attr::serializable()),
//   field(binding_name, other::attr::serializable()),
//   field(shader_resource_handle, other::attr::serializable()),
//   field(buf_usage, other::attr::serializable()),
//   field(buffer_type, other::attr::serializable()),
//   field(buffer_data, other::attr::serializable())
// )

#endif  // OTHER_RENDERER_GPU_RESOURCE_GPU_BUFFER_HPP