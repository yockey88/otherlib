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

  void editor_context::deselect_object(natural_t object_id) {
    if (current_selection.scene_ptr == nullptr) {
      CORE_LOG_ERROR("Cannot deselect object with ID {} because there is no active scene.", object_id);
      return;
    }

    auto& objects = current_selection.objects;
    auto it = std::ranges::find(objects, object_id);
    if (it != objects.end()) {
      objects.erase(it);
    } else {
      CORE_LOG_WARN("Tried to deselect object with ID {}, but it was not in the current selection.", object_id);
    }
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

  natural_t editor_context::register_viewport(const std::string_view name, const std::string_view render_pipeline_name) {
    auto& vp = viewports.emplace_back() = viewport_info{
      .viewport_name = std::format("{}:{}", name, render_pipeline_name),
    };
    viewport_definition def = {
      .name = std::string(name),
      .render_pipeline_name = std::string(render_pipeline_name),
      .cam = &editor_camera,
    };

    auto& kernel = driver->get_kernel();
    auto& rendering_sys = kernel.get_core_system<rendering_system>();
    vp.vp_id = rendering_sys.register_viewport(std::format("{}:{}", name, render_pipeline_name), def);
    return vp.vp_id;
  }

  natural_t editor_context::register_viewport(const std::string_view name, const std::string_view render_pipeline_name, camera* cam) {
    auto& vp = viewports.emplace_back() = viewport_info{
      .viewport_name = std::format("{}:{}", name, render_pipeline_name),
    };
    viewport_definition def = {
      .name = std::string(name),
      .render_pipeline_name = std::string(render_pipeline_name),
      .cam = cam,
    };

    auto& kernel = driver->get_kernel();
    auto& rendering_sys = kernel.get_core_system<rendering_system>();
    vp.vp_id = rendering_sys.register_viewport(std::format("{}:{}", name, render_pipeline_name), def);
    return vp.vp_id;
  }

  void editor_context::remove_viewport(natural_t vp_id) {
    auto vp_itr = std::ranges::find_if(viewports, [vp_id](const viewport_info& vp) { return vp.vp_id == vp_id; });
    if (vp_itr != viewports.end()) {
      auto& kernel = driver->get_kernel();
      auto& rendering_sys = kernel.get_core_system<rendering_system>();
      rendering_sys.remove_viewport(vp_id);
      viewports.erase(vp_itr);
    } else {
      CORE_LOG_WARN("Tried to remove viewport with ID '{}', but no matching viewport was found.", vp_id);
    }
  }

  void editor_context::remove_all_viewports() {
    auto& kernel = driver->get_kernel();
    auto& rendering_sys = kernel.get_core_system<rendering_system>();
    for (const auto& vp : viewports) {
      if (scene_viewport_handle != 0 && vp.vp_id == scene_viewport_handle) {
        continue;
      }
      rendering_sys.remove_viewport(vp.vp_id);
    }
    viewports.clear();
  }

  viewport& editor_context::get_viewport(natural_t vp_id) {
    auto vp_itr = std::ranges::find_if(viewports, [vp_id](const viewport_info& vp) { return vp.vp_id == vp_id; });
    if (vp_itr != viewports.end()) {
      auto& kernel = driver->get_kernel();
      auto& rendering_sys = kernel.get_core_system<rendering_system>();
      return rendering_sys.get_viewport(vp_itr->vp_id);
    } else {
      static viewport null_viewport{
        .id = 0,
        .name = "null",
        .pipeline = nullptr,
        .cam = nullptr,
        .texture = {},
      };
      return null_viewport;
    }
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