/**
 * \file model/skeleton.cpp
 **/
#include "model/skeleton.hpp"

#include <stack>

#include "model/animation.hpp"
#include "model/model.hpp"
#include "model/model_source.hpp"

namespace other {

  void bone_influence::add_bone_data(uint32_t bone_id, float weight) {
    if (current_bone_count >= kMaxBones) {
      // should never get here - more than 4 bone influences per vertex
      CORE_LOG_WARN("More than 4 bone influences on a vertex!");
      return;
    }

    bone_ids[current_bone_count] = bone_id;
    weights[current_bone_count] = weight;
    current_bone_count++;
  }

  void bone_influence::normalize() {
    double total = 0.0;
    for (uint32_t i = 0; i < kMaxBones; i++) {
      total += weights[i];
    }

    if (total > 0.0) {
      for (uint32_t i = 0; i < kMaxBones; i++) {
        weights[i] /= total;
      }
    }
  }

  uint32_t skeleton::add_bone_data(const std::string& name, uint32_t parent_id, const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale) {
    int32_t bone_id = static_cast<int32_t>(bones.size());
    CORE_LOG_DEBUG("Adding bone data: id={}, name='{}', parent_id={}", bone_id, name, parent_id);

    bone& b = bones.emplace_back();
    b.id = bone_id;
    b.parent_id = parent_id;
    b.name = name;

    parent_ids.emplace_back(parent_id);
    children_ids.emplace_back();
    bone_positions.emplace_back(position);
    bone_rotations.emplace_back(rotation);
    bone_scales.emplace_back(scale);
    local_bone_positions.emplace_back(glm::mat4(1.0f));
    final_bone_positions.emplace_back(glm::mat4(1.0f));
    return bone_id;
  }

  int32_t skeleton::get_bone_index(const std::string& name) const {
    for (uint32_t i = 0; i < bones.size(); ++i) {
      if (bones[i].name == name) {
        return i;
      }
    }
    return -1;
  }

  glm::mat4& skeleton::calculate_local_transform(uint32_t bone_id) {
    local_bone_positions[bone_id] = glm::translate(glm::mat4(1.0f), get_bone_position(bone_id)) *
      glm::toMat4(get_bone_rotation(bone_id)) *
      glm::scale(glm::mat4(1.0f), get_bone_scale(bone_id));
    return local_bone_positions[bone_id];
  }

  const glm::mat4& skeleton::get_local_transform(uint32_t bone_id) const {
    return local_bone_positions[bone_id];
  }

  glm::mat4& skeleton::calculate_final_transform(uint32_t bone_id) {
    final_bone_positions[bone_id] = calculate_local_transform(bone_id) * bones[bone_id].offset_matrix;
    return final_bone_positions[bone_id];
  }

  const glm::mat4& skeleton::get_final_transform(uint32_t bone_id) const {
    return final_bone_positions[bone_id];
  }

}  // namespace other