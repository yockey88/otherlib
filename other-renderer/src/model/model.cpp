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
#include "model/vertex.hpp"
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
      std::vector<float> vertex_data = build_vertex_buffer(vertices);
      std::vector<uint32_t> indices_data = build_index_buffer(indices);

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
      m->upload_vertex_buffer(name + "_model_vertices", vertices.size(), vertex_data.data(), vertex_data.size() * sizeof(float))
        .upload_index_buffer(name + "_model_indices", indices_data.size(), indices_data.data(), indices_data.size() * sizeof(uint32_t))
        .finalize_mesh();

      resource_handle vbuff = m->vertex_handle();

      OTHER_ASSERT(m->index_handle().has_value(), "Mesh must have an index buffer handle.");
      resource_handle ibuff = *m->index_handle();

      return { m->handle(), vbuff, ibuff };
    }

  }  // namespace

  mesh_node* model::get_node_by_name(const std::string& name) {
    for (auto& node : source->nodes) {
      if (node.name == name) {
        return &node;
      }
    }
    return nullptr;
  }

  submesh* model::get_submesh_by_name(const std::string& name) {
    for (auto& sm : source->submeshes) {
      if (sm.name == name) {
        return &sm;
      }
    }
    return nullptr;
  }

  void model::draw() {
    OTHER_ASSERT(source != nullptr, "SOURCE is null!");
    source->draw();
  }

  model::~model() {
    if (skel != nullptr) {
      arena_allocator<skeleton>{}.free(skel);
      skel = nullptr;
    }
  }

  // clang-format off
  model_source::model_source(const std::string& name, const std::vector<vertex>& vertices, const std::vector<index>& indices, const std::unordered_map<uint32_t, std::vector<triangle>>& triangle_map, 
                              const std::vector<submesh>& submeshes, const std::vector<mesh_node>& nodes, const std::vector<bone>& bones, const std::vector<material>& materials, const std::vector<animation>& animations, 
                              const glm::mat4& global_transform, const glm::mat4& inverse_global_transform, const bounding_box& bounds)
      // clang-format on
      : global_transform(global_transform), inverse_global_transform(inverse_global_transform), vertices(vertices), indices(indices), triangle_map(triangle_map), submeshes(submeshes), nodes(nodes), bones(bones),
        materials(materials), animations(animations), bounds(bounds), file_path("") {
    OTHER_ASSERT(!vertices.empty(), "Model source must have at least one vertex.");
    OTHER_ASSERT(!indices.empty(), "Model source must have at least one index.");
    OTHER_ASSERT(!submeshes.empty(), "Model source must have at least one submesh.");
    OTHER_ASSERT(!name.empty(), "Model source name cannot be empty.");

    CORE_LOG_DEBUG("Creating model source: {} with {} vertices, {} indices, {} submeshes, {} nodes, {} bones, {} materials, and {} animations.", name, vertices.size(), indices.size(), submeshes.size(), nodes.size(), bones.size(), materials.size(), animations.size());
    layout = vertex::get_buffer_layout();
    std::tie(mesh_handle, vertex_buffer_handle, index_buffer_handle) = get_mesh_handles(name, vertices, indices);
  }

  model_source::~model_source() {
  }

  model model_source::produce_model(const std::string& name, const std::vector<uint32_t>& submesh_idxs) {
    model m = {
      .name = name,
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

    m.skel = arena_allocator<skeleton>{}.allocate(name, this);

    return m;
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
    // clang-format off
    ref<model_source> src = make_ref<model_source>(file_path.filename().stem().string(), builder.vertices, builder.indices, builder.triangles, 
                                                                                         builder.submeshes, builder.nodes, builder.bones, builder.materials, builder.animations, 
                                                                                         builder.global_transform, builder.inverse_global_transform, builder.bounds);
    // clang-format on
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
    // clang-format off
    ref<model_source> src = make_ref<model_source>(name, builder.vertices, builder.indices, builder.triangles, 
                                                   builder.submeshes, builder.nodes, builder.bones, builder.materials, builder.animations, 
                                                   builder.global_transform, builder.inverse_global_transform, builder.bounds);
    // clang-format on

    natural_t hash = FNV(name);
    subsystem<renderer_backend>::get()->add_model_source(hash, src);

    CORE_LOG_DEBUG("Model source loaded with name: {} and hash: {}", name, hash);
    return { hash, src };
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

  void model_source::draw() {
    for (const auto& submesh : submeshes) {
      // if (index_buffer_handle.has_value() && index_buffer_handle->id != 0) {
      //   subsystem<renderer_backend>::get()->api()->draw_mesh(handle(), prim_type, vert_count, index_count, mesh::attribute_type::UNSIGNED_BYTE);
      // } else {
      //   subsystem<renderer_backend>::get()->api()->draw_mesh(handle(), prim_type, vert_count);
      // }
    }
  }

}  // namespace other