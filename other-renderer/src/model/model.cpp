/**
 * \file model/model.cpp
 **/
#include "model/model.hpp"

#include <cstdint>

#include <glm/glm.hpp>

#include "core/logger.hpp"
#include "core/profiler.hpp"
#include "math/bounding_box.hpp"

#include "gpu_resource/mesh.hpp"
#include "gpu_resource/shader.hpp"
#include "model/model_importer.hpp"
#include "renderer/render_graph.hpp"
#include "renderer/renderer_backend.hpp"

namespace other {
  namespace {

    std::vector<float> build_vertex_buffer(const std::vector<vertex>& vertices) {
      std::vector<float> raw_vertices;
      for (const auto& v : vertices) {
        raw_vertices.push_back(v.position.x);
        raw_vertices.push_back(v.position.y);
        raw_vertices.push_back(v.position.z);

        raw_vertices.push_back(v.normal.x);
        raw_vertices.push_back(v.normal.y);
        raw_vertices.push_back(v.normal.z);

        raw_vertices.push_back(v.tangent.x);
        raw_vertices.push_back(v.tangent.y);
        raw_vertices.push_back(v.tangent.z);

        raw_vertices.push_back(v.bitangent.x);
        raw_vertices.push_back(v.bitangent.y);
        raw_vertices.push_back(v.bitangent.z);

        raw_vertices.push_back(v.tex_coord.x);
        raw_vertices.push_back(v.tex_coord.y);
      }
      return raw_vertices;
    }

    std::vector<uint32_t> build_index_buffer(const std::vector<index>& indices) {
      std::vector<uint32_t> raw_indices;
      for (const auto& idx : indices) {
        raw_indices.push_back(idx.v0);
        raw_indices.push_back(idx.v1);
        raw_indices.push_back(idx.v2);
      }
      return raw_indices;
    }

    std::tuple<resource_handle, resource_handle, resource_handle> get_mesh_handles(const std::string& name, const std::vector<vertex>& vertices, const std::vector<index>& indices) {
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

      resource_handle mesh_handle = subsystem<renderer_backend>::get()->api()->create_resource(name + "_vertex_buffer", resource_type::MESH);
      (*subsystem<renderer_backend>::get()->api()->get_resource_as<mesh>(mesh_handle))
        .set_primitive_type(mesh::primitive_type::TRIANGLES);

      /// use the static vertex function instead of the one stored in the mesh because we know the layout here and don't
      ///      want the user to be able to get this wrong
      for (const auto& attr : vertex::get_buffer_layout()) {
        (*subsystem<renderer_backend>::get()->api()->get_resource_as<mesh>(mesh_handle))
          .add_attribute(attr);
      }

      mesh* m = subsystem<renderer_backend>::get()->api()->get_resource_as<mesh>(mesh_handle);
      m->upload_vertex_buffer(name + "_model_vertices", vertex_data.size() / vertex::get_buffer_layout().get_stride(), vertex_data.data(), vertex_data.size() * sizeof(float))
        .upload_index_buffer(name + "_model_indices", indices_data.size(), indices_data.data(), indices_data.size() * sizeof(uint32_t))
        .finalize_mesh();

      resource_handle vbuff = m->vertex_handle();

      OTHER_ASSERT(m->index_handle().has_value(), "Mesh must have an index buffer handle.");
      resource_handle ibuff = *m->index_handle();

      return { m->handle(), vbuff, ibuff };
    }

  }  // namespace

  void model::draw(shader* shader_ptr, const int32_t mat_idx, const glm::mat4& model_matrix) {
    PROFILE_SECTION("model::draw");

    // mesh* m = subsystem<renderer_backend>::get()->api()->get_resource_as<mesh>(source->get_mesh_handle());
    // if (m == nullptr) {
    //   return;
    // }

    // OTHER_ASSERT(shader_ptr != nullptr, "Shader pointer is null, cannot draw model.");

    // shader_ptr->bind()
    //   .set_uniform("model_matrix", model_matrix)
    //   .set_uniform("material_index", mat_idx);

    // m->bind();

    // auto submeshes = source->get_submeshes();
    // for (const auto& submesh_idx : submesh_indices) {
    //   draw_command cmd{
    //     .draw_mode = mesh::TRIANGLES,
    //     // .instance_count = 1,
    //     // .index_count = submeshes[submesh_idx].idx_cnt,
    //     // .vertex_offset = submeshes[submesh_idx].base_vertex,
    //   };
    //   subsystem<renderer_backend>::get()->api()->draw_mesh_instanced(m->handle(), cmd);
    // }

    // m->unbind();
    // shader_ptr->unbind();
  }

  // clang-format off
  model_source::model_source(const std::string& name, const std::vector<vertex>& vertices, const std::vector<index>& indices, const std::unordered_map<uint32_t, std::vector<triangle>>& triangle_map, 
                              const std::vector<submesh>& submeshes, const std::vector<mesh_node>& nodes, const bounding_box& bounds)
      // clang-format on
      : vertices(vertices), indices(indices), triangle_map(triangle_map), submeshes(submeshes), nodes(nodes), bounds(bounds), file_path("") {
    OTHER_ASSERT(!vertices.empty(), "Model source must have at least one vertex.");
    OTHER_ASSERT(!indices.empty(), "Model source must have at least one index.");
    OTHER_ASSERT(!submeshes.empty(), "Model source must have at least one submesh.");
    {
      std::vector<uint32_t> base_model_submesh_indices;
      for (uint32_t i = 0; i < submeshes.size(); ++i) {
        base_model_submesh_indices.push_back(submeshes[i].sub_mesh_id);
      }
      base_model = {
        .name = name,
        .source = this,
        .submesh_indices = std::move(base_model_submesh_indices),
      };
      OTHER_ASSERT(base_model.submesh_indices.size() == submeshes.size(), "Model source submesh indices size does not match submeshes size.");
    }

    layout = vertex::get_buffer_layout();

    auto [mhandle, vbuff, ibuff] = get_mesh_handles(name, vertices, indices);
    mesh_handle = mhandle;
    vertex_buffer_handle = vbuff;
    index_buffer_handle = ibuff;
  }

  model_source::model_source(const std::string& name, const std::vector<vertex>& vertices, const std::vector<index>& indices) {
    OTHER_ASSERT(!vertices.empty(), "Model source must have at least one vertex.");
    OTHER_ASSERT(!indices.empty(), "Model source must have at least one index.");

    this->vertices = vertices;
    this->indices = indices;

    base_model.name = name;
    base_model.source = this;
    base_model.submesh_indices = { 0 };

    submesh& submesh = submeshes.emplace_back();
    submesh.base_vertex = 0;
    submesh.base_idx = 0;
    submesh.mat_idx = 0;
    submesh.vert_cnt = static_cast<uint32_t>(vertices.size());
    submesh.idx_cnt = static_cast<uint32_t>(indices.size());
    // submesh.bounds = bounding_box::from_vertices(vertices);
    submesh.sub_mesh_id = 0;
    submesh.material_id = 0;
    submesh.rigged = false;
    submesh.transform = glm::mat4(1.0f);
    submesh.local_transform = glm::mat4(1.0f);
    submesh.name = name;

    layout = vertex::get_buffer_layout();

    auto [mhandle, vbuff, ibuff] = get_mesh_handles(name, vertices, indices);
    mesh_handle = mhandle;
    vertex_buffer_handle = vbuff;
    index_buffer_handle = ibuff;
  }

  model model_source::produce_model(const std::string& name, const std::vector<uint32_t>& submesh_idxs) {
    return model{
      .name = name,
      .source = this,
      .submesh_indices = submesh_idxs.empty() ? base_model.submesh_indices : submesh_idxs
    };
  }

  resource_handle model_source::get_mesh_handle() const {
    return mesh_handle;
  }

  std::pair<natural_t, ref<model_source>> model_source::load_model_source(const filepath& file_path) {
    CORE_LOG_DEBUG("Attempting to load model : {}", file_path.string());
    if (!std::filesystem::exists(file_path)) {
      CORE_LOG_ERROR("Model file does not exist: {}", file_path.string());
      return { 0, nullptr };
    }

    model_builder builder = model_importer::load_model_data(file_path);
    ref<model_source> src = make_ref<model_source>(file_path.filename().stem().string(), builder.vertices, builder.indices, builder.triangles, builder.submeshes, builder.nodes, builder.bounds);
    natural_t hash = FNV(file_path.string());
    subsystem<renderer_backend>::get()->add_model_source(hash, src);

    CORE_LOG_DEBUG("Model source loaded: {} with hash {}", file_path.string(), hash);
    return { hash, src };
  }

  std::pair<natural_t, ref<model_source>> model_source::load_model_source(const std::string& name, const std::vector<vertex>& vertices, const std::vector<index>& indices) {
    CORE_LOG_DEBUG("Attempting to load model source with name: {}", name);
    if (vertices.empty() || indices.empty()) {
      CORE_LOG_ERROR("Model source must have at least one vertex and one index.");
      return { 0, nullptr };
    }

    model_builder builder = model_importer::build_model_data(name, vertices, indices);

    ref<model_source> src = make_ref<model_source>(name, builder.vertices, builder.indices);
    natural_t hash = FNV(name);
    subsystem<renderer_backend>::get()->add_model_source(hash, src);

    CORE_LOG_DEBUG("Model source loaded with name: {} and hash: {}", name, hash);
    return { hash, src };
  }

}  // namespace other