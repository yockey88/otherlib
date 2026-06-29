/**
 * \file editor_context.cpp
 **/
#include "editor_context.hpp"

namespace other {

  void editor_context::select_object(natural_t object_id) {
    if (current_selection.scene_ptr == nullptr) {
      CORE_LOG_ERROR("Cannot select object with ID {} because there is no active scene.", object_id);
      return;
    }

    if (!current_selection.multiple_selection_enabled) {
      current_selection.objects.clear();
    }

    current_selection.objects.push_back(object_id);
  }

  bool editor_context::has_selection() const {
    return current_selection.scene_ptr != nullptr &&
      !current_selection.objects.empty();
  }

  bool editor_context::multi_select_enabled() const {
    return current_selection.multiple_selection_enabled;
  }

  bounding_box editor_context::get_selection_bounding_box() const {
    if (!has_selection()) {
      return {};
    }

    bounding_box result = bounding_box::empty;
    for (natural_t obj_id : current_selection.objects) {
      result = bounding_box::expand_to_include(result, current_selection.scene_ptr->get_bounding_box(obj_id));
    }

    if (result == bounding_box::empty) {
      result = bounding_box(glm::vec3(-1.f), glm::vec3(1.f));
    }
    return result;
  }

}  // namespace other