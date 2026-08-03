/**
 * \file asset/pipelines/material_pipeline.hpp
 **/
#ifndef OTHER_SCENE_ASSET_PIPELINES_MATERIAL_PIPELINE_HPP
#define OTHER_SCENE_ASSET_PIPELINES_MATERIAL_PIPELINE_HPP

#include "asset/asset_pipeline.hpp"

namespace other {

  class material_pipeline : public asset_pipeline {
   public:
    material_pipeline(event_system* events, asset_handler* handler)
        : asset_pipeline(events, handler) {}
    ~material_pipeline() = default;
  };

}  // namespace other

#endif  // OTHER_SCENE_ASSET_PIPELINES_MATERIAL_PIPELINE_HPP
