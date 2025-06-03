/**
 * \file model/model.cpp
 **/
#include "model/model.hpp"

#include <cstdint>

#include "renderer/renderer_backend.hpp"

namespace other {

  model model::create_model(const std::string& name, const std::vector<vertex>& vertices, const std::vector<index>& indices) {
    model m;
    m.name = name;
    m.layout = vertex::get_buffer_layout();

    std::vector<float> vertex_data;
    for (const auto& v : vertices) {
      vertex_data.push_back(v.position.x);
      vertex_data.push_back(v.position.y);
      vertex_data.push_back(v.position.z);

      vertex_data.push_back(v.normal.x);
      vertex_data.push_back(v.normal.y);
      vertex_data.push_back(v.normal.z);

      vertex_data.push_back(v.tangent.x);
      vertex_data.push_back(v.tangent.y);
      vertex_data.push_back(v.tangent.z);

      vertex_data.push_back(v.bitangent.x);
      vertex_data.push_back(v.bitangent.y);
      vertex_data.push_back(v.bitangent.z);

      vertex_data.push_back(v.tex_coord.x);
      vertex_data.push_back(v.tex_coord.y);
    }

    std::vector<uint32_t> indices_data;
    for (const auto& idx : indices) {
      indices_data.push_back(idx.v0);
      indices_data.push_back(idx.v1);
      indices_data.push_back(idx.v2);
    }

    m.vertex_buffer_handle = subsystem<renderer_backend>::get()->api()->create_resource(name + "_vertex_buffer", resource_type::MESH);
    (*subsystem<renderer_backend>::get()->api()->get_resource_as<mesh>(m.vertex_buffer_handle))
      .set_primitive_type(mesh::primitive_type::TRIANGLES);

    /// use the static vertex function instead of the one stored in the mesh because we know the layout here and don't
    ///      want the user to be able to get this wrong
    for (const auto& attr : vertex::get_buffer_layout()) {
      (*subsystem<renderer_backend>::get()->api()->get_resource_as<mesh>(m.vertex_buffer_handle))
        .add_attribute(attr);
    }

    (*subsystem<renderer_backend>::get()->api()->get_resource_as<mesh>(m.vertex_buffer_handle))
      .upload_vertex_buffer(name + "_model_vertices", vertex_data.size() / m.layout.get_stride(), vertex_data.data(), vertex_data.size() * sizeof(float))
      .upload_index_buffer(name + "_model_indices", indices_data.size(), indices_data.data(), indices_data.size() * sizeof(uint32_t))
      .finalize_mesh();

    return m;
  }

}  // namespace other