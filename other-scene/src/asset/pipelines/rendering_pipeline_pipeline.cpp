/**
 * \file asset/pipelines/rendering_pipeline_pipeline.cpp
 **/
#include "asset/pipelines/rendering_pipeline_pipeline.hpp"

#include "asset/asset_pipeline.hpp"

namespace other {

  void rendering_pipeline_pipeline::on_load_complete(asset* asset_ptr) {
    OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in on_load_complete");
    CORE_LOG_DEBUG("Rendering pipeline [{}] loaded successfully.", asset_ptr->id);
  }

  void rendering_pipeline_pipeline::on_load_failed(asset* asset_ptr, const std::string& error_message) {
    OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in on_load_failed");
    CORE_LOG_ERROR("Failed to load rendering pipeline [{}]: {}", asset_ptr->id, error_message);
  }

  void rendering_pipeline_pipeline::on_unload_complete(asset* asset_ptr) {
    OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in on_unload_complete");
    CORE_LOG_DEBUG("Rendering pipeline [{}] unloaded successfully.", asset_ptr->id);
  }

  void rendering_pipeline_pipeline::on_unload_failed(asset* asset_ptr, const std::string& error_message) {
    OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in on_unload_failed");
    CORE_LOG_ERROR("Failed to unload rendering pipeline [{}]: {}", asset_ptr->id, error_message);
  }

  void rendering_pipeline_pipeline::on_pipeline_poll() {
  }

}  // namespace other
