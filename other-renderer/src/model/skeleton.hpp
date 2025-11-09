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
    int32_t parent_id = -1;
    int32_t id = -1;

    uint32_t submesh_idx = 0;
    std::string name;

    size_t vertex_group_start = 0;
    size_t vertex_group_size = 0;

    glm::mat4 offset_matrix = glm::mat4(1.0f);
    std::vector<bone_weight> weights;
  };

  struct bone_info {
    uint32_t id;
    glm::mat4 offset;
  };

  struct bone_influence {
    uint32_t vertex_id = 0;

    constexpr static size_t kMaxBones = 4;
    size_t current_bone_count = 0;
    int32_t bone_ids[kMaxBones] = { -1, -1, -1, -1 };
    float weights[kMaxBones] = { 0.f, 0.f, 0.f, 0.f };

    void add_bone_data(uint32_t bone_id, float weight);
    void normalize();
  };

  class model_source;
  struct animation;

  struct skeleton {
    std::string name;

    glm::mat4 skeleton_transform = glm::mat4(1.0f);

    std::set<std::string> bone_names;
    std::vector<bone> bones;
    std::vector<bone_influence> bone_influence;

    std::vector<uint32_t> parent_ids;
    std::vector<std::vector<uint32_t>> children_ids;

    std::vector<glm::vec3> bone_positions;
    std::vector<glm::quat> bone_rotations;
    std::vector<glm::vec3> bone_scales;

    std::vector<glm::mat4> local_bone_positions;
    std::vector<glm::mat4> final_bone_positions;

    std::vector<glm::mat4> model_space_rest_transforms;
    std::vector<glm::mat4> model_space_rest_inverse_transforms;

    const glm::vec3& get_bone_position(uint32_t bone_id) const { return bone_positions[bone_id]; }
    const glm::quat& get_bone_rotation(uint32_t bone_id) const { return bone_rotations[bone_id]; }
    const glm::vec3& get_bone_scale(uint32_t bone_id) const { return bone_scales[bone_id]; }

    uint32_t add_bone_data(const std::string& name, uint32_t parent_id, const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale);
    int32_t get_bone_index(const std::string& name) const;

    glm::mat4& calculate_local_transform(uint32_t bone_id);
    const glm::mat4& get_local_transform(uint32_t bone_id) const;

    glm::mat4& calculate_final_transform(uint32_t bone_id);
    const glm::mat4& get_final_transform(uint32_t bone_id) const;
  };

}  // namespace other

#endif  // OTHER_RENDERER_MODEL_SKELETON_HPP