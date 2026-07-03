/**
 * \file editor_context.cpp
 **/
#include "editor_context.hpp"

#include "editor_driver.hpp"
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

  ImTextureID editor_context::get_texture_id_by_name(const std::string_view name) const {
    if (name.empty()) {
      return 0;
    }

    return 0;
  }

  ImTextureID editor_context::get_pipeline_texture_id(const std::string_view pipeline_name, const std::string_view texture_name) const {
    if (pipeline_name.empty() || texture_name.empty()) {
      return 0;
    }

    auto* pl = driver->get_renderer().get_pipeline(std::string{ pipeline_name });
    if (pl == nullptr) {
      CORE_LOG_WARN("Pipeline '{}' not found when trying to get texture ID for texture '{}'", pipeline_name, texture_name);
      return 0;
    }

    return pl->get_texture_id(std::string{ texture_name });
  }

  ImVec2 editor_context::get_pipeline_texture_size(const std::string_view pipeline_name, const std::string_view texture_name) const {
    if (pipeline_name.empty() || texture_name.empty()) {
      return ImVec2(0.f, 0.f);
    }

    auto* pl = driver->get_renderer().get_pipeline(std::string{ pipeline_name });
    if (pl == nullptr) {
      CORE_LOG_WARN("Pipeline '{}' not found when trying to get texture size for texture '{}'", pipeline_name, texture_name);
      return ImVec2(0.f, 0.f);
    }

    glm::ivec2 size = pl->get_texture_size(std::string{ texture_name });
    return ImVec2(static_cast<float>(size.x), static_cast<float>(size.y));
  }

}  // namespace other