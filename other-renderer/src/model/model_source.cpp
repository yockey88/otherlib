/**
 * \file model/model_source.cpp
 **/
#include "model/model_source.hpp"

#include "renderer/renderer_backend.hpp"

namespace other {

  namespace detail {

    std::vector<float> build_vertex_buffer(const std::vector<vertex>& vertices);
    std::vector<uint32_t> build_index_buffer(const std::vector<index>& indices);
    std::tuple<resource_handle, resource_handle, resource_handle> get_mesh_handles(const std::string& name, const std::vector<vertex>& vertices, const std::vector<index>& indices);

  }  // namespace detail

  // clang-format off
  model_source::model_source(const std::string& name, const std::vector<vertex>& vertices, const std::vector<index>& indices, const std::vector<triangle>& triangles,
                              const std::vector<submesh>& submeshes, const std::vector<mesh_node>& nodes, const std::vector<material>& materials, const std::vector<animation>& animations, 
                              const skeleton& skel, const glm::mat4& global_transform, const glm::mat4& inverse_global_transform, const bounding_box& bounds)
      // clang-format on
      : name(name), global_transform(global_transform), inverse_global_transform(inverse_global_transform), vertices(vertices), indices(indices), triangles(triangles), submeshes(submeshes), nodes(nodes),
        materials(materials), animations(animations), bounds(bounds), file_path("") {
    OTHER_ASSERT(!vertices.empty(), "Model source must have at least one vertex.");
    OTHER_ASSERT(!indices.empty(), "Model source must have at least one index.");
    OTHER_ASSERT(!submeshes.empty(), "Model source must have at least one submesh.");
    OTHER_ASSERT(!name.empty(), "Model source name cannot be empty.");

    this->skel = arena_allocator<skeleton>{}.allocate();

    this->skel->name = skel.name;
    this->skel->bones = skel.bones;
    this->skel->parent_ids = skel.parent_ids;
    this->skel->children_ids = skel.children_ids;
    this->skel->bone_positions = skel.bone_positions;
    this->skel->bone_rotations = skel.bone_rotations;
    this->skel->bone_scales = skel.bone_scales;
    this->skel->local_bone_positions = skel.local_bone_positions;
    this->skel->final_bone_positions = skel.final_bone_positions;

    for (uint32_t vidx = 0; vidx < vertices.size(); ++vidx) {
      vertex& v = this->vertices[vidx];

      double total_weight = v.bone_weights.x + v.bone_weights.y + v.bone_weights.z + v.bone_weights.w;
      if (total_weight > 0.0) {
        v.bone_weights.x = static_cast<float>(v.bone_weights.x / total_weight);
        v.bone_weights.y = static_cast<float>(v.bone_weights.y / total_weight);
        v.bone_weights.z = static_cast<float>(v.bone_weights.z / total_weight);
        v.bone_weights.w = static_cast<float>(v.bone_weights.w / total_weight);
      }
    }

    CORE_LOG_DEBUG("Creating model source: {} with {} vertices, {} indices, {} submeshes, {} nodes, {} bones, {} materials, and {} animations.", name, vertices.size(), indices.size(), submeshes.size(), nodes.size(), skel.bones.size(), materials.size(), animations.size());
    layout = vertex::get_buffer_layout();
    std::tie(mesh_handle, vertex_buffer_handle, index_buffer_handle) = detail::get_mesh_handles(name, vertices, indices);
  }

  model_source::~model_source() {
    arena_allocator<skeleton>{}.free(skel);
  }

  void model_source::destroy_resources() {
    if (subsystem<renderer_backend>::get()->api()->resource_exists(index_buffer_handle)) {
      subsystem<renderer_backend>::get()->api()->destroy_resource(index_buffer_handle);
      CORE_LOG_DEBUG("Destroyed index buffer resource for model source '{}'", name);
    }
    if (subsystem<renderer_backend>::get()->api()->resource_exists(vertex_buffer_handle)) {
      subsystem<renderer_backend>::get()->api()->destroy_resource(vertex_buffer_handle);
      CORE_LOG_DEBUG("Destroyed vertex buffer resource for model source '{}'", name);
    }
    if (subsystem<renderer_backend>::get()->api()->resource_exists(mesh_handle)) {
      subsystem<renderer_backend>::get()->api()->destroy_resource(mesh_handle);
      CORE_LOG_DEBUG("Destroyed mesh resource for model source '{}'", name);
    }
  }

  model model_source::produce_model(const std::string& name, const std::vector<uint32_t>& submesh_idxs) {
    model m = {
      .name = name.empty() ? this->name + "_instance_" + std::to_string(num_models_produced++) : name,
      .source = this,
      .submesh_indices = submesh_idxs.empty() ?
        (std::ranges::iota_view{ 0u, (uint32_t)submeshes.size() } | std::ranges::to<std::vector<uint32_t>>()) :
        submesh_idxs
    };

    for (const auto& sm_idx : m.submesh_indices) {
      const auto& sm = submeshes[sm_idx];
      auto [itr, inserted] = m.local_submesh_transforms.insert({ sm_idx, sm.local_transform });
      OTHER_ASSERT(inserted, "Failed to insert local submesh transform for submesh index {}", sm_idx);
    }

    m.skel = skel;

    return m;
  }

  resource_handle model_source::get_mesh_handle() const {
    return mesh_handle;
  }

  size_t model_source::get_num_vertices() const {
    return vertices.size();
  }

  size_t model_source::get_num_indices() const {
    return indices.size();
  }

  animation* model_source::get_animation_by_name(const std::string& name) {
    for (auto& anim : animations) {
      if (anim.name == name) {
        return &anim;
      }
    }
    return nullptr;
  }

  animation* model_source::get_animation(size_t index) {
    OTHER_ASSERT(index < animations.size(), "Animation index out of bounds");
    return &animations[index];
  }

  namespace detail {

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

        raw_vertices.push_back(static_cast<float>(v.bone_ids.x));
        raw_vertices.push_back(static_cast<float>(v.bone_ids.y));
        raw_vertices.push_back(static_cast<float>(v.bone_ids.z));
        raw_vertices.push_back(static_cast<float>(v.bone_ids.w));

        raw_vertices.push_back(v.bone_weights.x);
        raw_vertices.push_back(v.bone_weights.y);
        raw_vertices.push_back(v.bone_weights.z);
        raw_vertices.push_back(v.bone_weights.w);
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
      std::vector<float> vertex_data = vertex::to_gpu_buffer(vertices);
      std::vector<uint32_t> indices_data = index::to_gpu_buffer(indices);

      resource_handle mesh_handle = subsystem<renderer_backend>::get()->api()->create_resource(name + "_vertex_buffer", resource_type::MESH);
      (*subsystem<renderer_backend>::get()->api()->get_resource_as<mesh>(mesh_handle))
        .set_primitive_type(mesh::primitive_type::TRIANGLES);

      /// use the static vertex function instead of the one stored in the mesh because we know the layout here and don't
      ///      want the user to be able to get this wrong
      buffer_layout layout = vertex::get_buffer_layout();
      for (const auto& attr : layout) {
        (*subsystem<renderer_backend>::get()->api()->get_resource_as<mesh>(mesh_handle))
          .add_attribute(attr);
      }

      mesh* m = subsystem<renderer_backend>::get()->api()->get_resource_as<mesh>(mesh_handle);
      m->upload_vertex_buffer(name + "_model_vertices", gpu_buffer::usage::STATIC, vertices.size(), vertex_data.data(), vertex_data.size() * sizeof(float))
        .upload_index_buffer(name + "_model_indices", gpu_buffer::usage::STATIC, indices_data.size(), indices_data.data(), indices_data.size() * sizeof(uint32_t))
        .finalize_mesh();

      resource_handle vbuff = m->vertex_handle();

      OTHER_ASSERT(m->index_handle().has_value(), "Mesh must have an index buffer handle.");
      resource_handle ibuff = *m->index_handle();

      return { m->handle(), vbuff, ibuff };
    }

  }  // namespace detail
}  // namespace other