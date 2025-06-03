/**
 * \file model/vertex.hpp
 **/
#ifndef OTHER_RENDERER_MODEL_VERTEX_HPP
#define OTHER_RENDERER_MODEL_VERTEX_HPP

#include <glm/glm.hpp>

#include "serialization/reflection.hpp"

namespace other {

  struct vertex_attribute {
    OTHER_REFLECTABLE(vertex_attribute);

    std::string name = "";
    value_type type = value_type::EMPTY_TYPE;

    size_t idx = 0;  // index of the attribute in the vertex buffer
    size_t size = 0;
    size_t offset = 0;

    size_t num_components();

    vertex_attribute() {}
    vertex_attribute(value_type type, const std::string& name)
        : name(name), type(type) {
      size = num_components();
    }
  };

  struct buffer_layout {
    OTHER_REFLECTABLE(buffer_layout);

    buffer_layout() = default;
    buffer_layout(std::initializer_list<vertex_attribute> attributes);
    buffer_layout(const buffer_layout& layout);
    buffer_layout& operator=(const buffer_layout& layout);
    buffer_layout(buffer_layout&& layout) noexcept;
    buffer_layout& operator=(buffer_layout&& layout) noexcept;

    using attribute_list = std::vector<vertex_attribute>;

    uint32_t get_stride() const;
    const std::vector<vertex_attribute>& get_elements() const;

    std::vector<uint32_t> raw_layout() const;
    uint32_t count() const;

    [[nodiscard]] attribute_list::iterator begin();
    [[nodiscard]] attribute_list::iterator end();
    [[nodiscard]] attribute_list::const_iterator begin() const;
    [[nodiscard]] attribute_list::const_iterator end() const;

    uint32_t stride = 0;
    std::vector<vertex_attribute> elements;

   private:
    std::vector<uint32_t> raw_layout_cache;

    void calculate_offsets();
  };

  struct vertex {
    OTHER_REFLECTABLE(vertex);

    glm::vec3 position = { 0, 0, 0 };
    glm::vec3 normal = { 0, 0, 1 };
    glm::vec3 tangent = { 1, 0, 0 };
    glm::vec3 bitangent = { 0, 1, 0 };
    glm::vec2 tex_coord = { 0, 0 };

    vertex() = default;

    static std::vector<uint32_t> layout;
    static size_t stride();

    static buffer_layout get_buffer_layout();
  };

  struct index {
    OTHER_REFLECTABLE(index);
    int32_t v0 = 0;
    int32_t v1 = 0;
    int32_t v2 = 0;
  };

  struct triangle {
    OTHER_REFLECTABLE(triangle);
    vertex v0;
    vertex v1;
    vertex v2;
  };

}  // namespace other

OTHER_REFLECT(
  other::vertex,
  field(position, other::attr::serializable())
  // field(normal, other::attr::serializable())
  // field(tangent, other::attr::serializable()),
  // field(bitangent, other::attr::serializable()),
  // field(tex_coord, other::attr::serializable())
)

OTHER_REFLECT(
  other::index,
  field(v0, other::attr::serializable()),
  field(v1, other::attr::serializable()),
  field(v2, other::attr::serializable())
)

OTHER_REFLECT(
  other::triangle,
  field(v0, other::attr::serializable()),
  field(v1, other::attr::serializable()),
  field(v2, other::attr::serializable())
)

OTHER_REFLECT(
  other::vertex_attribute,
  field(name, other::attr::serializable()),
  field(type, other::attr::serializable()),
  field(size, other::attr::serializable()),
  field(offset, other::attr::serializable())
)

OTHER_REFLECT(
  other::buffer_layout,
  field(stride, other::attr::serializable())
)

#endif  // OTHER_RENDERER_MODEL_VERTEX_HPP