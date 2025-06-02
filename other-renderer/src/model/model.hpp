/**
 * \file model/model.hpp
 **/
#ifndef OTHER_RENDERER_MODEL_MODEL_HPP
#define OTHER_RENDERER_MODEL_MODEL_HPP

#include "renderer/mesh.hpp"
#include "renderer/renderer_resource.hpp"

#include "model/vertex.hpp"
#include "model/vertex_buffer.hpp"

namespace other {

  struct model {
    OTHER_REFLECTABLE(model);

    std::string name;
    buffer_layout layout;
    resource_handle vertex_buffer_handle;

    static model create_model(const std::string& name, const std::vector<vertex>& vertices, const std::vector<index>& indices);

    model() = default;
  };

}  // namespace other

OTHER_REFLECT(
  other::model,
  field(name, other::attr::serializable()),
  field(layout, other::attr::serializable())
)

#endif  // OTHER_RENDERER_MODEL_MODEL_HPP