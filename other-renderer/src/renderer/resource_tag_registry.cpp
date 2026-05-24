/**
 * \file renderer/resource_tag_registry.cpp
 **/
#include "renderer/resource_tag_registry.hpp"

namespace other {

  void resource_tag_registry::register_binder(resource_tag tag, resource_tag_binder_fn_t resource_binder) {
    if (auto itr = binders.find(tag); itr != binders.end()) {
      CORE_LOG_ERROR("Tag already registered! {}", tag.value());
      return;
    }
    auto [itr, success] = binders.emplace(tag, resource_binder);
    OTHER_ASSERT(success, "Failed to insert binder for tag {}", tag.value());
  }

  resource_tag_binder_fn_t resource_tag_registry::find(resource_tag tag) const {
    auto itr = binders.find(tag);
    if (itr == binders.end()) {
      CORE_LOG_ERROR("Failed to find binder for resource tag : {}", tag.value());
      return nullptr;
    }
    return itr->second;
  }

}  // namespace other