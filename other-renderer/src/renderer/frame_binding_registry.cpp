/**
 * \file renderer/frame_binding_registry.cpp
 **/
#include "renderer/frame_binding_registry.hpp"

namespace other {

  void frame_binding_registry::register_per_frame(resource_tag tag, per_frame_producer_fn producer) {
    if (auto itr = per_frame_binders.find(tag); itr != per_frame_binders.end()) {
      CORE_LOG_ERROR("Tag already registered! {}", tag.value());
      return;
    }
    auto [itr, success] = per_frame_binders.emplace(tag, producer);
    OTHER_ASSERT(success, "Failed to insert binder for tag {}", tag.value());
  }

  void frame_binding_registry::register_per_draw(resource_tag tag, per_draw_producer_fn producer) {
    if (auto itr = per_draw_binders.find(tag); itr != per_draw_binders.end()) {
      CORE_LOG_ERROR("Tag already registered! {}", tag.value());
      return;
    }
    auto [itr, success] = per_draw_binders.emplace(tag, producer);
    OTHER_ASSERT(success, "Failed to insert binder for tag {}", tag.value());
  }

  void frame_binding_registry::register_per_instance(resource_tag tag, per_instance_producer_fn producer) {
    if (auto itr = per_instance_binders.find(tag); itr != per_instance_binders.end()) {
      CORE_LOG_ERROR("Tag already registered! {}", tag.value());
      return;
    }
    auto [itr, success] = per_instance_binders.emplace(tag, producer);
    OTHER_ASSERT(success, "Failed to insert binder for tag {}", tag.value());
  }

  per_frame_producer_fn frame_binding_registry::find_per_frame(resource_tag tag) const {
    auto itr = per_frame_binders.find(tag);
    if (itr == per_frame_binders.end()) {
      return nullptr;
    }
    return itr->second;
  }

  per_draw_producer_fn frame_binding_registry::find_per_draw(resource_tag tag) const {
    auto itr = per_draw_binders.find(tag);
    if (itr == per_draw_binders.end()) {
      return nullptr;
    }
    return itr->second;
  }

  per_instance_producer_fn frame_binding_registry::find_per_instance(resource_tag tag) const {
    auto itr = per_instance_binders.find(tag);
    if (itr == per_instance_binders.end()) {
      return nullptr;
    }
    return itr->second;
  }

}  // namespace other