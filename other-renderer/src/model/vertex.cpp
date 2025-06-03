/**
 * \file model/vertex.cpp
 **/
#include "model/vertex.hpp"

namespace other {
  namespace {

    static std::vector<uint32_t> actual_layout = { 3, 3, 3, 3, 2 };

    static size_t get_stride() {
      size_t stride = 0;
      for (const auto& component : actual_layout) {
        stride += component;
      }
      return stride;
    }

  }  // namespace

  size_t vertex_attribute::num_components() {
    switch (type) {
      case VEC2:
        return 2;
      case VEC3:
        return 3;
      case VEC4:
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
    elements = std::vector<vertex_attribute>(attributes);
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

  const std::vector<vertex_attribute>& buffer_layout::get_elements() const {
    return elements;
  }

  std::vector<uint32_t> buffer_layout::raw_layout() const {
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

  std::vector<uint32_t> vertex::layout = actual_layout;
  size_t vertex::stride = get_stride();

  buffer_layout vertex::get_buffer_layout() {
    return {
      vertex_attribute(value_type::VEC3, "position"),
      vertex_attribute(value_type::VEC3, "normal"),
      vertex_attribute(value_type::VEC3, "tangent"),
      vertex_attribute(value_type::VEC3, "bitangent"),
      vertex_attribute(value_type::VEC2, "tex_coord"),
    };
  }

}  // namespace other