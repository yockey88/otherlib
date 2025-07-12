/**
 * \file model/model.hpp
 **/
#ifndef OTHER_RENDERER_MODEL_MODEL_HPP
#define OTHER_RENDERER_MODEL_MODEL_HPP

#include "math/bounding_box.hpp"
#include "serialization/reflection.hpp"

#include "gpu_resource/mesh.hpp"
#include "gpu_resource/renderer_resource.hpp"
#include "model/vertex.hpp"
#include "model/vertex_buffer.hpp"

namespace other {

  struct shader;
  class model_source;

  struct model {
    std::string name;
    model_source* source = nullptr;
    std::vector<uint32_t> submesh_indices;
  };

  class model_source : public ref_counted {
   public:
    // clang-format off
   model_source(const std::string& name, const std::vector<vertex>& vertices, const std::vector<index>& indices, const std::unordered_map<uint32_t, std::vector<triangle>>& triangle_map,
                const std::vector<submesh>& submeshes, const std::vector<mesh_node>& nodes, const bounding_box& bounds);
    // clang-format on

    model produce_model(const std::string& name, const std::vector<uint32_t>& submesh_idxs = {});

    resource_handle get_mesh_handle() const;

    static std::pair<natural_t, ref<model_source>> load_model_source(const filepath& file_path);
    static std::pair<natural_t, ref<model_source>> load_model_source(const std::string& name, const std::vector<vertex>& vertices, const std::vector<index>& indices);

    inline const std::unordered_map<uint32_t, std::vector<triangle>>& get_triangles() const {
      return triangle_map;
    }
    inline const std::vector<submesh>& get_submeshes() const {
      return submeshes;
    }

    void draw();

   private:
    friend struct model;

    size_t num_models_produced = 0;

    model base_model;

    buffer_layout layout;

    std::vector<vertex> vertices;
    std::vector<index> indices;
    std::unordered_map<uint32_t, std::vector<triangle>> triangle_map;

    std::vector<submesh> submeshes;
    std::vector<mesh_node> nodes;

    resource_handle mesh_handle;
    resource_handle vertex_buffer_handle;
    resource_handle index_buffer_handle;

    bounding_box bounds = bounding_box::empty;
    std::string file_path;

    /*
    // std::vector<Bone> bones;
    // std::vector<BoneInfl> bone_influences;
    // std::map<uint32_t , std::vector<Triangle>> triangles;
    // mutable Scope<Skeleton> skeleton = nullptr;
    */

    std::vector<float> raw_vertices;
    std::vector<uint32_t> raw_indices;
  };

}  // namespace other

OTHER_REFLECT(
  other::model,
  field(name, other::attr::serializable()),
  field(submesh_indices, other::attr::serializable())
)

#endif  // OTHER_RENDERER_MODEL_MODEL_HPP