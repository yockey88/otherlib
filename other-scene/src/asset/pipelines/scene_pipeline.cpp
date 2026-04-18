/**
 * \file asset/pipelines/scene_pipeline.cpp
 **/
#include "asset/pipelines/scene_pipeline.hpp"

#include "scene/scene.hpp"

namespace other {

  void scene_pipeline::on_load_complete(asset* asset_ptr) {
    OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in scene_pipeline");
    OTHER_ASSERT(asset_ptr->asset_type == asset::SCENE, "Asset type is not SCENE in scene_pipeline");
    OTHER_ASSERT(scene_ptr != nullptr, "Scene pointer in pipeline is null in on_load_complete");

    scene_ptr->asset_id = asset_ptr->id;
    CORE_LOG_DEBUG("Scene pipeline finished for scene: {} with asset ID: {}", scene_ptr->id, asset_ptr->id);
    get_events().trigger_event("scene.asset-loaded", scene_ptr->id);
  }

  void scene_pipeline::on_load_failed(asset* asset_ptr, const std::string& error_message) {
    OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in scene_pipeline");
    OTHER_ASSERT(asset_ptr->asset_type == asset::SCENE, "Asset type is not SCENE in scene_pipeline");
    OTHER_ASSERT(scene_ptr != nullptr, "Scene pointer in pipeline is null in on_load_failed");

    CORE_LOG_ERROR("Scene pipeline failed for scene: {} with asset ID: {}. Error: {}", scene_ptr->id, asset_ptr->id, error_message);
    // get_events().trigger_event("scene.asset-load-failed", std::make_tuple(scene_ptr->id, error_message));
  }

  void scene_pipeline::on_unload_complete(asset* asset_ptr) {
    OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in scene_pipeline");
    OTHER_ASSERT(asset_ptr->asset_type == asset::SCENE, "Asset type is not SCENE in scene_pipeline");
    OTHER_ASSERT(scene_ptr != nullptr, "Scene pointer in pipeline is null in on_unload_complete");

    CORE_LOG_DEBUG("Scene pipeline finished for scene: {} with asset ID: {}", scene_ptr->id, asset_ptr->id);
    get_events().trigger_event("scene.asset-unloaded", scene_ptr->id);
  }

  void scene_pipeline::on_unload_failed(asset* asset_ptr, const std::string& error_message) {
    OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in scene_pipeline");
    OTHER_ASSERT(asset_ptr->asset_type == asset::SCENE, "Asset type is not SCENE in scene_pipeline");
    OTHER_ASSERT(scene_ptr != nullptr, "Scene pointer in pipeline is null in on_unload_failed");

    CORE_LOG_ERROR("Pipeline failed to unload scene asset ID: {} with error: {}", asset_ptr->id, error_message);
    // get_events().trigger_event("scene.asset-unload-failed", std::make_tuple(scene_ptr->id, error_message));
  }

  void scene_pipeline::on_pipeline_poll() {
  }

}  // namespace other