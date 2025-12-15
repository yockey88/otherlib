/**
 * \file model/model.hpp
 **/
#ifndef OTHER_RENDERER_MODEL_MODEL_HPP
#define OTHER_RENDERER_MODEL_MODEL_HPP

#include "core/defines.hpp"
#include "math/bounding_box.hpp"
#include "serialization/reflection.hpp"

#include "gpu_resource/mesh.hpp"
#include "gpu_resource/renderer_resource.hpp"
#include "model/animation.hpp"
#include "model/material.hpp"
#include "model/skeleton.hpp"
#include "model/vertex.hpp"
#include "model/vertex_buffer.hpp"

namespace other {

  struct shader;
  class model_source;

  struct model {
    std::string name;

    model_source* source = nullptr;
    skeleton* skel = nullptr;

    std::vector<uint32_t> submesh_indices;

    std::vector<glm::mat4> bone_matrices;
    std::unordered_map<uint32_t, glm::mat4> local_submesh_transforms;

    mesh_node* get_node_by_name(const std::string& name);
    submesh* get_submesh_by_name(const std::string& name);

    void draw();
  };

  class model_source : public ref_counted {
   public:
    // clang-format off
   model_source(const std::string& name, const std::vector<vertex>& vertices, const std::vector<index>& indices, const std::vector<triangle>& triangle_map,
                const std::vector<submesh>& submeshes, const std::vector<mesh_node>& nodes, const std::vector<material>& materials, const std::vector<animation>& animations, 
                const skeleton& skeleton, const glm::mat4& global_transform, const glm::mat4& inverse_global_transform, const bounding_box& bounds);
    // clang-format on
    ~model_source();

    model produce_model(const std::string& name, const std::vector<uint32_t>& submesh_idxs = {});

    resource_handle get_mesh_handle() const;

    static std::pair<natural_t, ref<model_source>> load_model_source(const filepath& file_path);
    static std::pair<natural_t, ref<model_source>> load_model_source(const std::string& name, const std::vector<vertex>& vertices, const std::vector<index>& indices);

    /// non-const overloads (not all are provided)
    inline std::vector<submesh>& get_submeshes() { return submeshes; }
    inline std::vector<mesh_node>& get_nodes() { return nodes; }
    /// const overloads
    inline const glm::mat4& get_global_transform() const { return global_transform; }
    inline const glm::mat4& get_inverse_global_transform() const { return inverse_global_transform; }
    inline const std::vector<triangle>& get_triangles() const { return triangles; }
    inline const std::vector<submesh>& get_submeshes() const { return submeshes; }
    inline const std::vector<mesh_node>& get_nodes() const { return nodes; }
    inline const std::vector<material>& get_materials() const { return materials; }
    inline const std::vector<animation>& get_animations() const { return animations; }

    animation* get_animation_by_name(const std::string& name);
    animation* get_animation(size_t index);

    void draw();

   private:
    friend struct model;

    size_t num_models_produced = 0;

    skeleton* skel = nullptr;

    glm::mat4 global_transform = glm::mat4(1.0f);
    glm::mat4 inverse_global_transform = glm::mat4(1.0f);

    buffer_layout layout;

    /// \todo this is going to have to move to something with stable addressing consider how many pointers exist to
    ///        the data in these vectors

    /// base mesh data and geometry
    std::vector<vertex> vertices;
    std::vector<index> indices;
    std::vector<triangle> triangles;

    /// model structure data and transform relations for organizating geometry
    std::vector<submesh> submeshes;
    std::vector<mesh_node> nodes;

    /// high level model data
    std::vector<material> materials;
    std::vector<animation> animations;

    resource_handle mesh_handle;
    resource_handle vertex_buffer_handle;
    resource_handle index_buffer_handle;

    bounding_box bounds = bounding_box::empty;
    std::string file_path;

    std::vector<float> raw_vertices;
    std::vector<uint32_t> raw_indices;
  };

}  // namespace other

OTHER_REFLECT(
  other::model,
  field(name, other::attr::serializable()),
  field(submesh_indices, other::attr::serializable()),
  field(bone_matrices, other::attr::serializable()),
  field(local_submesh_transforms, other::attr::serializable())
)

#endif  // OTHER_RENDERER_MODEL_MODEL_HPP