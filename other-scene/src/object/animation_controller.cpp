/**
 * \file object/animation_controller.cpp
 **/
#include "object/animation_controller.hpp"

#include <queue>
#include <stack>

#include "gpu_resource/mesh.hpp"
#include "model/animation.hpp"
#include "model/model.hpp"
#include "model/vertex.hpp"
#include "renderer/gpu_structs.hpp"

namespace other {

  void animation_controller::update(double delta_time) {
    // OTHER_ASSERT(anim_ptr != nullptr, "Animation pointer is null in animation_controller");
    // OTHER_ASSERT(model_ptr != nullptr, "Model pointer is null in animation_controller");

    // model_ptr->bone_matrices.clear();
    // model_ptr->bone_matrices.resize(model_ptr->skel->bones.size(), glm::mat4(1.0f));

    // animation_accumulator += anim_ptr->ticks_per_second * delta_time * animation_speed;
    // anim_ptr->current_time = fmod(animation_accumulator, anim_ptr->duration);

    // for (auto& channel : anim_ptr->channels) {
    //   const auto& _ = channel.get_transform_at_time(anim_ptr->current_time);
    // }

    // skeleton* model_skeleton = model_ptr->skel;
    // OTHER_ASSERT(model_skeleton != nullptr, "Model has no skeleton in animation_controller");

    // glm::mat4 global_inverse_transform = model_ptr->source->get_inverse_global_transform();
    // glm::mat4 global_model_transform = model_ptr->source->get_global_transform();
    // for (uint32_t b = 0; b < model_ptr->skel->bones.size(); ++b) {
    //   const auto& bone = model_ptr->skel->bones[b];

    //   auto node_itr = std::ranges::find_if(model_ptr->source->get_nodes(), [&bone](const auto& node) { return node.name == bone.name; });
    //   OTHER_ASSERT(node_itr != model_ptr->source->get_nodes().end(), "Bone node not found in model nodes");

    //   glm::mat4 local_bone_transform = node_itr->local_transform;
    //   auto ch_itr = std::ranges::find_if(anim_ptr->channels, [&bone](const auto& channel) { return channel.node_name == bone.name; });
    //   if (ch_itr != anim_ptr->channels.end()) {
    //     local_bone_transform = ch_itr->local_transform;
    //   }

    //   glm::mat4 parent_transform = glm::mat4(1.0f);
    //   if (model_ptr->skel->parent_ids[bone.id] != -1) {
    //     parent_transform = model_ptr->skel->local_bone_positions[model_ptr->skel->parent_ids[bone.id]];
    //   }

    //   model_ptr->skel->local_bone_positions[bone.id] = parent_transform * local_bone_transform;
    //   model_ptr->skel->final_bone_positions.at(bone.id) =
    //     global_model_transform *
    //     model_ptr->skel->local_bone_positions[bone.id] * bone.offset_matrix *
    //     global_inverse_transform;
    // }

    // size_t num_bones = std::min(model_ptr->skel->bones.size(), static_cast<size_t>(gpu::kMaxMaterials));
    // for (size_t i = 0; i < num_bones; ++i) {
    //   model_ptr->bone_matrices[i] = model_ptr->skel->final_bone_positions[i];
    // }
  }

  double animation_controller::get_interpolation_factor(double current_time, double start_time, double end_time) const {
    if (end_time - start_time == 0.0) {
      return 0.0;
    }
    return (current_time - start_time) / (end_time - start_time);
  }

}  // namespace other