/**
 * \file tools/mesh-baker/main.cpp
 **/
#include <cstdint>
#include <iostream>
#include <print>

#include "core/defines.hpp"

#include "model/model_importer.hpp"

using namespace other;

int main(int argc, char** argv) {
  /// \todo command line
  filepath input_path = "tests/resources/models/suzanne3.fbx";
  std::println("Input path: {}", input_path.string());

  if (!std::filesystem::exists(input_path)) {
    std::println(std::cerr, "Input file does not exist: {}", input_path.string());
    return 1;
  }

  model_builder builder = model_importer::load_model_data(input_path);
  std::println("Loaded model with {} vertices, {} indices, {} submeshes", builder.vertices.size(), builder.indices.size(), builder.submeshes.size());

  std::vector<double> vertex_data;
  for (const auto& v : builder.vertices) {
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
  for (const auto& idx : builder.indices) {
    indices_data.push_back(idx.v0);
    indices_data.push_back(idx.v1);
    indices_data.push_back(idx.v2);
  }

  std::vector<uint8_t> buffer = {};
  /**
    | <mesh-block> | <mesh-length> | <vertex-index-data> | <submeshes> | <nodes> |
    |------------------------------|---------------------------------------------|
    | 4 bytes      | 4 bytes       |                     |             |         |

   vertex-index-data
    | <num-vertices> | <vertices> (stride of 7 doubles each) | <index-data> |
    |-----------------------------------------------------------------------|
    | 4 bytes        | 7 * <num-vertices> * 8 bytes          |              |

   index-data
    | <num-indices> | <indices> (stride of 3 uint32_t each) |
    |-------------------------------------------------------|
    | 4 bytes       | 3 * <num-indices> * 4 bytes           |

   submeshes
    | <num-submeshes> | <submesh-data> |
    |----------------------------------|
    | 4 bytes         |                |

   nodes
    | <num-nodes> | <node-data> |
    |---------------------------|
    | 4 bytes     |             |

    triangle-data
    | <num-triangles> | <triangle-data> |
    |-----------------------------------|
    | 4 bytes         |                 |

   submesh-data
    | <base-vertex> | <base-idx> | <mat-idx> | <idx-cnt> | <vert-cnt> | <local-transform> | <bounds> | <sub-mesh-id> | <material-id> | <name-length> | <name> | <rigged> |
    |--------------------------------------------------------------------------------------------------------------------------------------------------------------------|
    | 4 bytes       | 4 bytes    | 4 bytes   | 4 bytes   | 4 bytes    | 64 bytes          | 24 bytes | 8 bytes       | 8 bytes       | 4 bytes       |        | 1 byte   |

   node-data
    | <parent> | <num-children> | <children>         | <num-submeshes> | <submesh-ids>       | <local-transform> | <name-length> | <name> |
    |-------------------------------------------------------------------------------------------------------------------------------------|
    | 4 bytes  | 4 bytes        | 4 * <num-children> | 4 bytes         | 4 * <num-submeshes> | 64 bytes          | 4 bytes       |        |
   **/

  size_t vertex_size = vertex_data.size() * sizeof(double);
  size_t index_size = indices_data.size() * sizeof(uint32_t);

  uint32_t num_vertices = builder.vertices.size();
  const uint8_t* vertex_bytes = reinterpret_cast<const uint8_t*>(vertex_data.data());
  const uint8_t* num_vertices_bytes = reinterpret_cast<const uint8_t*>(&num_vertices);

  uint32_t num_indices = builder.indices.size();
  const uint8_t* index_bytes = reinterpret_cast<const uint8_t*>(indices_data.data());
  const uint8_t* num_indices_bytes = reinterpret_cast<const uint8_t*>(&num_indices);

  uint32_t num_submeshes = builder.submeshes.size();
  const uint8_t* num_submeshes_bytes = reinterpret_cast<const uint8_t*>(&num_submeshes);

  std::vector<uint8_t> submesh_buffer;
  for (const auto& submesh : builder.submeshes) {
    const uint8_t* base_vertex_bytes = reinterpret_cast<const uint8_t*>(&submesh.base_vertex);
    const uint8_t* base_idx_bytes = reinterpret_cast<const uint8_t*>(&submesh.base_idx);
    const uint8_t* mat_idx_bytes = reinterpret_cast<const uint8_t*>(&submesh.mat_idx);
    const uint8_t* idx_cnt_bytes = reinterpret_cast<const uint8_t*>(&submesh.idx_cnt);
    const uint8_t* vert_cnt_bytes = reinterpret_cast<const uint8_t*>(&submesh.vert_cnt);
    const uint8_t* local_transform_bytes = reinterpret_cast<const uint8_t*>(&submesh.local_transform);
    const uint8_t* bounds_min_bytes = reinterpret_cast<const uint8_t*>(&submesh.bounds.min);
    const uint8_t* bounds_max_bytes = reinterpret_cast<const uint8_t*>(&submesh.bounds.max);
    const uint8_t* sub_mesh_id_bytes = reinterpret_cast<const uint8_t*>(&submesh.sub_mesh_id);
    const uint8_t* material_id_bytes = reinterpret_cast<const uint8_t*>(&submesh.material_id);

    uint32_t name_length = static_cast<uint32_t>(submesh.name.size());
    const uint8_t* name_length_bytes = reinterpret_cast<const uint8_t*>(&name_length);
    const uint8_t* name_bytes = reinterpret_cast<const uint8_t*>(submesh.name.data());

    uint8_t rigged_byte = submesh.rigged ? 1 : 0;
    const uint8_t* rigged_bytes = reinterpret_cast<const uint8_t*>(&rigged_byte);

    submesh_buffer.append_range(std::span(base_vertex_bytes, sizeof(uint32_t)));
    submesh_buffer.append_range(std::span(base_idx_bytes, sizeof(uint32_t)));
    submesh_buffer.append_range(std::span(mat_idx_bytes, sizeof(uint32_t)));
    submesh_buffer.append_range(std::span(idx_cnt_bytes, sizeof(uint32_t)));
    submesh_buffer.append_range(std::span(vert_cnt_bytes, sizeof(uint32_t)));
    submesh_buffer.append_range(std::span(local_transform_bytes, sizeof(glm::mat4)));
    submesh_buffer.append_range(std::span(bounds_min_bytes, sizeof(glm::vec3)));
    submesh_buffer.append_range(std::span(bounds_max_bytes, sizeof(glm::vec3)));
    submesh_buffer.append_range(std::span(sub_mesh_id_bytes, sizeof(natural_t)));
    submesh_buffer.append_range(std::span(material_id_bytes, sizeof(natural_t)));
    submesh_buffer.append_range(std::span(name_length_bytes, sizeof(uint32_t)));
    submesh_buffer.append_range(std::span(name_bytes, name_length));
    submesh_buffer.append_range(std::span(rigged_bytes, sizeof(uint8_t)));
  }

  uint32_t num_nodes = builder.nodes.size();
  const uint8_t* num_nodes_bytes = reinterpret_cast<const uint8_t*>(&num_nodes);

  std::vector<uint8_t> node_buffer;
  for (const auto& node : builder.nodes) {
    const uint8_t* parent_bytes = reinterpret_cast<const uint8_t*>(&node.parent);

    uint32_t num_children = node.children.size();
    const uint8_t* num_children_bytes = reinterpret_cast<const uint8_t*>(&num_children);

    std::vector<uint8_t> children_bytes_vec;
    for (const auto& child : node.children) {
      const uint8_t* child_bytes = reinterpret_cast<const uint8_t*>(&child);
      children_bytes_vec.append_range(std::span(child_bytes, sizeof(uint32_t)));
    }

    uint32_t num_submeshes = node.sub_meshes.size();
    const uint8_t* num_submeshes_bytes = reinterpret_cast<const uint8_t*>(&num_submeshes);

    std::vector<uint8_t> submesh_ids_bytes_vec;
    for (const auto& submesh_id : node.sub_meshes) {
      const uint8_t* submesh_id_bytes = reinterpret_cast<const uint8_t*>(&submesh_id);
      submesh_ids_bytes_vec.append_range(std::span(submesh_id_bytes, sizeof(uint32_t)));
    }

    const uint8_t* local_transform_bytes = reinterpret_cast<const uint8_t*>(&node.local_transform);

    uint32_t name_length = static_cast<uint32_t>(node.name.size());
    const uint8_t* name_length_bytes = reinterpret_cast<const uint8_t*>(&name_length);
    const uint8_t* name_bytes = reinterpret_cast<const uint8_t*>(node.name.data());

    node_buffer.append_range(std::span(parent_bytes, sizeof(uint32_t)));
    node_buffer.append_range(std::span(num_children_bytes, sizeof(uint32_t)));
    node_buffer.append_range(std::span(children_bytes_vec.data(), children_bytes_vec.size()));
    node_buffer.append_range(std::span(num_submeshes_bytes, sizeof(uint32_t)));
    node_buffer.append_range(std::span(submesh_ids_bytes_vec.data(), submesh_ids_bytes_vec.size()));
    node_buffer.append_range(std::span(local_transform_bytes, sizeof(glm::mat4)));
    node_buffer.append_range(std::span(name_length_bytes, sizeof(uint32_t)));
    node_buffer.append_range(std::span(name_bytes, name_length));
  }

  size_t vertex_index_size = sizeof(uint32_t) + vertex_size + sizeof(uint32_t) + index_size;
  size_t submeshes_size = sizeof(uint32_t) + submesh_buffer.size();
  size_t nodes_size = sizeof(uint32_t) + node_buffer.size();

  uint32_t mesh_length = static_cast<uint32_t>(vertex_index_size + submeshes_size + nodes_size);
  const uint8_t* mesh_length_bytes = reinterpret_cast<const uint8_t*>(&mesh_length);

  /// header
  buffer.append_range(std::vector<uint8_t>{ 'M', 'E', 'S', 'H' });
  buffer.append_range(std::span(mesh_length_bytes, sizeof(uint32_t)));
  /// vertex-index-data
  buffer.append_range(std::span(num_vertices_bytes, sizeof(uint32_t)));
  buffer.append_range(std::span(vertex_bytes, vertex_size));
  buffer.append_range(std::span(num_indices_bytes, sizeof(uint32_t)));
  buffer.append_range(std::span(index_bytes, index_size));
  /// submeshes
  buffer.append_range(std::span(num_submeshes_bytes, sizeof(uint32_t)));
  buffer.append_range(std::span(submesh_buffer.data(), submesh_buffer.size()));
  /// nodes
  buffer.append_range(std::span(num_nodes_bytes, sizeof(uint32_t)));
  buffer.append_range(std::span(node_buffer.data(), node_buffer.size()));

  filepath output_path = "resources/models/suzanne3.omesh";
  {
    std::println("Writing output mesh to: {}", output_path.string());
    std::println("     - file size: {} bytes", buffer.size());
    std::ofstream output_file(output_path, std::ios::binary | std::ios::trunc);
    output_file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
  }

  return 0;
}