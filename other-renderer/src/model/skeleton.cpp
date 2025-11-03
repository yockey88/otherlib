/**
 * \file model/skeleton.cpp
 **/
#include "model/skeleton.hpp"

#include <stack>

#include "model/animation.hpp"
#include "model/model.hpp"

namespace other {

  std::vector<glm::mat4> skeleton::calculate_bone_matrices(const glm::mat4& parent_transform, animation* anim_ptr) {
    OTHER_ASSERT(source_ptr != nullptr, "Model source pointer is null in skeleton::update_bone_matrices");
    std::vector<bone>& bones = source_ptr->get_bones();
    if (bones.empty()) {
      return {};
    }

    std::vector<submesh>& submeshes = source_ptr->get_submeshes();
    std::vector<mesh_node>& nodes = source_ptr->get_nodes();

    std::vector<glm::mat4> bone_matrices(bones.size(), glm::mat4(1.0f));
#if 0
    calculate_bone_matrix(nodes[0], parent_transform, nodes, bones, bone_matrices, anim_ptr);
#else
    const glm::mat4& inverse_global_transform = source_ptr->get_inverse_global_transform();

    std::stack<std::pair<uint32_t, glm::mat4> > node_stack;
    node_stack.push({ 0, parent_transform });
    do {
      auto [current_node_idx, current_parent_transform] = node_stack.top();
      node_stack.pop();

      auto& node = nodes[current_node_idx];
      glm::mat4 local_node_transform = node.local_transform;
      if (anim_ptr != nullptr) {
        auto ch = std::ranges::find_if(anim_ptr->channels, [&node](const animation_channel& channel) { return channel.node_name == node.name; });
        if (ch != anim_ptr->channels.end()) {
          local_node_transform = ch->local_transform;
        }
      }

      glm::mat4 global_transform = current_parent_transform * local_node_transform;
      CORE_LOG_DEBUG("Calculated node '{}' global transform:\n{}", node.name, global_transform);

      auto bone_itr = std::ranges::find_if(bones, [&node](const bone& b) { return b.name == node.name; });
      if (bone_itr != bones.end()) {
        bone_matrices[bone_itr->id] = inverse_global_transform * global_transform * bone_itr->offset_matrix;
      }

      for (const auto& child_idx : node.children) {
        node_stack.push({ child_idx, global_transform });
      }
    } while (!node_stack.empty());
#endif
    return bone_matrices;
  }

  void skeleton::calculate_bone_matrix(const mesh_node& node, const glm::mat4& parent_transform, const std::vector<mesh_node>& nodes, const std::vector<bone>& bones, std::vector<glm::mat4>& out_bone_matrices, animation* anim_ptr) {
    glm::mat4 local_node_transform = node.local_transform;
    if (anim_ptr != nullptr) {
      auto ch = std::ranges::find_if(anim_ptr->channels, [&node](const animation_channel& channel) { return channel.node_name == node.name; });
      if (ch != anim_ptr->channels.end()) {
        local_node_transform = ch->local_transform;
      }
    }

    glm::mat4 global_transform = parent_transform * local_node_transform;
    CORE_LOG_DEBUG("Calculated node '{}' global transform:\n{}", node.name, global_transform);

    auto bone_itr = std::ranges::find_if(bones, [&node](const bone& b) { return b.name == node.name; });
    if (bone_itr != bones.end()) {
      out_bone_matrices[bone_itr->id] = source_ptr->get_inverse_global_transform() * global_transform * bone_itr->offset_matrix;
      // CORE_LOG_DEBUG("Calculated bone matrix for bone '{}':\n{}", bone_itr->name, b);
    }

    for (const auto& child_idx : node.children) {
      calculate_bone_matrix(nodes[child_idx], global_transform, nodes, bones, out_bone_matrices, anim_ptr);
    }
  }

}  // namespace other