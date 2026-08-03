/**
 * \file asset/pipelines/model_source_pipeline.hpp
 **/
#ifndef OTHER_SCENE_ASSET_PIPELINES_MODEL_SOURCE_PIPELINE_HPP
#define OTHER_SCENE_ASSET_PIPELINES_MODEL_SOURCE_PIPELINE_HPP

#include "model/model_importer.hpp"

#include "asset/asset_pipeline.hpp"

namespace other {

  class model_source_pipeline : public asset_pipeline {
   public:
    model_source_pipeline(event_system* events, asset_handler* handler)
        : asset_pipeline(events, handler) {}
    ~model_source_pipeline() override = default;

    /// in-memory entry mode (add_model_source_asset): pre-built data the loader consumes instead of importing a file
    model_data data;
  };

}  // namespace other

#endif  // OTHER_SCENE_ASSET_PIPELINES_MODEL_SOURCE_PIPELINE_HPP