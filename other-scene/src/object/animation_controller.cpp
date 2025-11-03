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

namespace other {

  void animation_controller::update(double delta_time) {
    OTHER_ASSERT(anim_ptr != nullptr, "Animation pointer is null in animation_controller");
    OTHER_ASSERT(model_ptr != nullptr, "Model pointer is null in animation_controller");

    animation_accumulator += anim_ptr->ticks_per_second * delta_time * animation_speed;
    anim_ptr->current_time = fmod(animation_accumulator, anim_ptr->duration);

    for (auto& channel : anim_ptr->channels) {
      const auto& _ = channel.get_transform_at_time(anim_ptr->current_time);
    }

    skeleton* model_skeleton = model_ptr->skel;
    OTHER_ASSERT(model_skeleton != nullptr, "Model has no skeleton in animation_controller");

    model_ptr->bone_matrices = model_skeleton->calculate_bone_matrices(glm::mat4(1.0f), anim_ptr);

    for (auto& channel : anim_ptr->channels) {
      submesh* model_submesh = model_ptr->get_submesh_by_name(channel.node_name);
      if (model_submesh == nullptr) {
        continue;
      }

      auto itr = model_ptr->local_submesh_transforms.find(model_submesh->sub_mesh_id);
      if (itr != model_ptr->local_submesh_transforms.end()) {
        itr->second = channel.local_transform;
      }
    }
  }

  double animation_controller::get_interpolation_factor(double current_time, double start_time, double end_time) const {
    if (end_time - start_time == 0.0) {
      return 0.0;
    }
    return (current_time - start_time) / (end_time - start_time);
  }

}  // namespace other