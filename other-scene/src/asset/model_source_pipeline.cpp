/**
 * \file asset/model_source_pipeline.cpp
 **/
#include "asset/model_source_pipeline.hpp"

#include "model/model_importer.hpp"
#include "renderer/renderer_backend.hpp"

namespace other {

  void model_source_pipeline::on_load_complete(asset* asset_ptr) {
    OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in model_source_pipeline");
    OTHER_ASSERT(asset_ptr->asset_type == asset::MODEL_SOURCE, "Asset type is not MODEL_SOURCE in model_source_pipeline");
    PROFILE_SECTION("model_importer::load_model_data--create-model-source");

    std::string name = asset_ptr->load_path.filename().stem().string();
    CORE_LOG_DEBUG("Pipeline finished successfully for asset ID: {}", asset_ptr->id);
    CORE_LOG_DEBUG("Creating model source with name: {}", name);

    // clang-format off
    ref<model_source> src = make_ref<model_source>(name, builder.vertices, builder.indices, builder.triangles,
                                                   builder.submeshes, builder.nodes, builder.materials, builder.animations,
                                                   builder.skel, builder.global_transform, builder.inverse_global_transform, builder.bounds);
    // clang-format on
    if (!src) {
      CORE_LOG_ERROR("Failed to create model source for file: {}", asset_ptr->load_path.string());
      return;
    }

    subsystem<renderer_backend>::get()->add_model_source(asset_ptr->path_hash, src);
    CORE_LOG_INFO("Model source loaded and registered: {} with hash {}", asset_ptr->load_path.string(), asset_ptr->path_hash);
  }

  void model_source_pipeline::on_load_failed(asset* asset_ptr, const std::string& error_message) {
  }

  void model_source_pipeline::on_unload_complete(asset* asset_ptr) {
    OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in model_source_pipeline");
    OTHER_ASSERT(asset_ptr->asset_type == asset::MODEL_SOURCE, "Asset type is not MODEL_SOURCE in model_source_pipeline");

    CORE_LOG_DEBUG("Pipeline unload finished successfully for asset ID: {}", asset_ptr->id);

    subsystem<renderer_backend>::get()->remove_model_source(asset_ptr->path_hash);
    CORE_LOG_DEBUG("Model source unloaded and unregistered: {} with hash {}", asset_ptr->load_path.string(), asset_ptr->path_hash);
  }

  void model_source_pipeline::on_unload_failed(asset* asset_ptr, const std::string& error_message) {
  }

  void model_source_pipeline::on_pipeline_poll() {}

}  // namespace other