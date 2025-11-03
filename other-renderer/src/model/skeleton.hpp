/**
 * \file model/skeleton.hpp
 **/
#ifndef OTHER_RENDERER_MODEL_SKELETON_HPP
#define OTHER_RENDERER_MODEL_SKELETON_HPP

#include "gpu_resource/renderer_resource.hpp"
#include "model/vertex.hpp"

#include "glm/fwd.hpp"

namespace other {

  struct bone_weight {
    uint32_t bone_id = 0;
    float weight = 0.0f;
  };

  struct bone {
    uint32_t submesh_idx = 0;
    int32_t id = -1;
    std::string name;

    size_t vertex_group_start = 0;
    size_t vertex_group_size = 0;

    glm::mat4 offset_matrix = glm::mat4(1.0f);
    std::vector<bone_weight> weights;
  };

  class model_source;
  struct animation;

  struct skeleton {
    skeleton(const std::string& name, model_source* source_ptr)
        : model_name(name), source_ptr(source_ptr) {
      /// create bone buffer matrix here?
    }
    ~skeleton() = default;

    std::string model_name;
    model_source* source_ptr = nullptr;

    std::vector<glm::mat4> calculate_bone_matrices(const glm::mat4& parent_transform, animation* anim_ptr = nullptr);

    void calculate_bone_matrix(const mesh_node& node, const glm::mat4& parent_transform, const std::vector<mesh_node>& nodes, const std::vector<bone>& bones, std::vector<glm::mat4>& out_bone_matrices, animation* anim_ptr = nullptr);

    // resource_handle gpu_bone_buffer;
  };

}  // namespace other

#endif  // OTHER_RENDERER_MODEL_SKELETON_HPP