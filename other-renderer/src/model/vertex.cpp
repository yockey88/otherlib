/**
 * \file model/vertex.cpp
 **/
#include "model/vertex.hpp"

#include "core/enum_formatter.hpp"

namespace other {

  size_t vertex_attribute::num_components() {
    switch (type) {
      case VEC2:
        return 2;
      case VEC3:
        return 3;
      case VEC4:
        return 4;

      case IVEC2:
        return 2;
      case IVEC3:
        return 3;
      case IVEC4:
        return 4;

      case MAT3:
        return 3 * 3;
      case MAT4:
        return 4 * 4;

      default:
        OTHER_ASSERT(false, "Unimplemented vertex attribute type: {}", type);
    }
  }

  buffer_layout::buffer_layout(std::initializer_list<vertex_attribute> attributes) {
    elements = ostd::vector<vertex_attribute>(attributes);
    calculate_offsets();
  }

  buffer_layout::buffer_layout(const buffer_layout& layout)
      : stride(layout.stride), elements(layout.elements) {
    calculate_offsets();
  }

  buffer_layout& buffer_layout::operator=(const buffer_layout& layout) {
    if (this != &layout) {
      stride = layout.stride;
      elements = layout.elements;
      calculate_offsets();
    }
    return *this;
  }

  buffer_layout::buffer_layout(buffer_layout&& layout) noexcept
      : stride(layout.stride), elements(std::move(layout.elements)) {
    raw_layout_cache = std::move(layout.raw_layout_cache);
    layout.raw_layout_cache.clear();
    calculate_offsets();
  }

  buffer_layout& buffer_layout::operator=(buffer_layout&& layout) noexcept {
    if (this != &layout) {
      stride = layout.stride;
      elements = std::move(layout.elements);
      raw_layout_cache = std::move(layout.raw_layout_cache);
      layout.raw_layout_cache.clear();
      calculate_offsets();
    }
    return *this;
  }

  uint32_t buffer_layout::get_stride() const {
    return stride;
  }

  const std::span<const vertex_attribute> buffer_layout::get_elements() const {
    return elements;
  }

  ostd::vector<uint32_t> buffer_layout::raw_layout() const {
    return raw_layout_cache;
  }

  uint32_t buffer_layout::count() const {
    return static_cast<uint32_t>(elements.size());
  }

  buffer_layout::attribute_list::iterator buffer_layout::begin() {
    return elements.begin();
  }

  buffer_layout::attribute_list::iterator buffer_layout::end() {
    return elements.end();
  }

  buffer_layout::attribute_list::const_iterator buffer_layout::begin() const {
    return elements.begin();
  }

  buffer_layout::attribute_list::const_iterator buffer_layout::end() const {
    return elements.end();
  }

  void buffer_layout::calculate_offsets() {
    size_t idx = 0;
    uint32_t offset = 0;
    for (auto& element : elements) {
      element.idx = idx++;
      element.offset = offset;
      offset += element.size;

      raw_layout_cache.push_back(element.size);
    }
    stride = offset;
  }

  size_t vertex::stride() {
    return get_buffer_layout().get_stride();
  }

  buffer_layout vertex::get_buffer_layout() {
    return {
      vertex_attribute(value_type::VEC3, "position"),
      vertex_attribute(value_type::VEC3, "normal"),
      vertex_attribute(value_type::VEC3, "tangent"),
      vertex_attribute(value_type::VEC3, "bitangent"),
      vertex_attribute(value_type::VEC2, "tex_coord"),
      vertex_attribute(value_type::IVEC4, "bone_ids"),
      vertex_attribute(value_type::VEC4, "bone_weights"),
    };
  }

  ostd::vector<float> vertex::to_gpu_buffer(const vertex& v) {
    ostd::vector<float> buffer;
    buffer.push_back(v.position.x);
    buffer.push_back(v.position.y);
    buffer.push_back(v.position.z);
    buffer.push_back(v.normal.x);
    buffer.push_back(v.normal.y);
    buffer.push_back(v.normal.z);
    buffer.push_back(v.tangent.x);
    buffer.push_back(v.tangent.y);
    buffer.push_back(v.tangent.z);
    buffer.push_back(v.bitangent.x);
    buffer.push_back(v.bitangent.y);
    buffer.push_back(v.bitangent.z);
    buffer.push_back(v.tex_coord.x);
    buffer.push_back(v.tex_coord.y);
    buffer.push_back(static_cast<float>(v.bone_ids.x));
    buffer.push_back(static_cast<float>(v.bone_ids.y));
    buffer.push_back(static_cast<float>(v.bone_ids.z));
    buffer.push_back(static_cast<float>(v.bone_ids.w));
    buffer.push_back(v.bone_weights.x);
    buffer.push_back(v.bone_weights.y);
    buffer.push_back(v.bone_weights.z);
    buffer.push_back(v.bone_weights.w);
    return buffer;
  }

  ostd::vector<float> vertex::to_gpu_buffer(const std::span<const vertex> v) {
    ostd::vector<float> buffer;
    for (const auto& vert : v) {
      buffer.append_range(to_gpu_buffer(vert));
    }
    return buffer;
  }

  ostd::vector<uint32_t> index::to_gpu_buffer(const index& idx) {
    return { idx.v0, idx.v1, idx.v2 };
  }

  ostd::vector<uint32_t> index::to_gpu_buffer(const std::span<const index> indices) {
    ostd::vector<uint32_t> buffer;
    for (const auto& idx : indices) {
      buffer.append_range(to_gpu_buffer(idx));
    }
    return buffer;
  }

}  // namespace other