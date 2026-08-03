/**
 * \file model/model_source.cpp
 **/
#include "model/model_source.hpp"

#include <ranges>

#include "core/logger.hpp"

namespace other {

  model_source::model_source(model_data&& data)
      : data(std::move(data)) {
    OTHER_ASSERT(this->data.valid(), "Model source constructed from invalid model data.");
    OTHER_ASSERT(!this->data.name.empty(), "Model source name cannot be empty.");

    CORE_LOG_DEBUG("Creating model source: {} with {} vertices, {} indices, {} submeshes, {} nodes, {} joints, {} materials, and {} clips.",
                   this->data.name, this->data.vertices.size(), this->data.indices.size(), this->data.submeshes.size(),
                   this->data.nodes.size(), this->data.skel.joints.size(), this->data.materials.size(), this->data.clips.size());
  }

  const animation_clip* model_source::find_clip(std::string_view name) const {
    for (const animation_clip& clip : data.clips) {
      if (clip.name == name) {
        return &clip;
      }
    }
    return nullptr;
  }

  model_source::~model_source() {
    OTHER_ASSERT(!uploaded(), "Model source '{}' destroyed with live gpu resources; renderer_backend::destroy_model must run first.", data.name);
  }

  model model_source::produce_model(const std::string& name, std::span<const uint32_t> submesh_idxs) {
    model m = {
      .name = name.empty() ? data.name + "_instance_" + std::to_string(num_models_produced++) : name,
      .source = this,
      .submesh_indices = submesh_idxs.empty() ?
        (std::ranges::iota_view{ 0u, (uint32_t)data.submeshes.size() } | std::ranges::to<ostd::vector<uint32_t>>()) :
        ostd::vector<uint32_t>(submesh_idxs.begin(), submesh_idxs.end())
    };

    for (const auto& sm_idx : m.submesh_indices) {
      const submesh& sm = data.submeshes[sm_idx];
      auto [itr, inserted] = m.local_submesh_transforms.insert({ sm_idx, sm.local_transform });
      OTHER_ASSERT(inserted, "Failed to insert local submesh transform for submesh index {}", sm_idx);
    }

    m.skel = &data.skel;

    return m;
  }

}  // namespace other
