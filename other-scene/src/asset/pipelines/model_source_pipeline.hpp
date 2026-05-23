/**
 * \file asset/pipelines/model_source_pipeline.hpp
 **/
#ifndef OTHER_SCENE_ASSET_PIPELINES_MODEL_SOURCE_PIPELINE_HPP
#define OTHER_SCENE_ASSET_PIPELINES_MODEL_SOURCE_PIPELINE_HPP

#include "model/model.hpp"
#include "model/model_importer.hpp"

#include "asset/asset_pipeline.hpp"

namespace other {

  class model_source_pipeline : public asset_pipeline {
   public:
    model_source_pipeline(event_system* events, asset_handler* handler)
        : asset_pipeline(events, handler) {}
    ~model_source_pipeline() override = default;

    model_builder builder;
  };

}  // namespace other

#endif  // OTHER_SCENE_ASSET_PIPELINES_MODEL_SOURCE_PIPELINE_HPP