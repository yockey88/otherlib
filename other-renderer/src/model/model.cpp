/**
 * \file model/model.cpp
 **/
#include "model/model.hpp"

#include "renderer/renderer_backend.hpp"

namespace other {

  model model::create_model(const std::string& name, const std::vector<vertex>& vertices, const std::vector<index>& indices) {
    model m;
    m.name = name;
    m.layout = vertex::get_buffer_layout();

    m.vertex_buffer_handle = subsystem<renderer_backend>::get()->api()->create_resource(name + "_vertex_buffer", resource_type::MESH);
    (*subsystem<renderer_backend>::get()->api()->get_resource_as<mesh>(m.vertex_buffer_handle))
      .set_primitive_type(mesh::primitive_type::TRIANGLES)
      .add_attribute("position", mesh::attribute_type::FLOAT, 3, 0)
      .add_attribute("normal", mesh::attribute_type::FLOAT, 3, 3)
      .add_attribute("tangent", mesh::attribute_type::FLOAT, 3, 6)
      .add_attribute("bitangent", mesh::attribute_type::FLOAT, 3, 9)
      .add_attribute("tex_coords", mesh::attribute_type::FLOAT, 2, 6)
      .upload_vertex_buffer("model_vertices", vertices.size(), vertices.data(), sizeof(vertex))
      .upload_index_buffer("model_indices", indices.size(), indices.data(), sizeof(index))
      .finalize_mesh();

    return m;
  }

}  // namespace other