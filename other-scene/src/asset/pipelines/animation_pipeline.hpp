/**
 * \file asset/pipelines/animation_pipeline.hpp
 **/
#ifndef OTHER_SCENE_ASSET_PIPELINES_ANIMATION_PIPELINE_HPP
#define OTHER_SCENE_ASSET_PIPELINES_ANIMATION_PIPELINE_HPP

#include "asset/asset_pipeline.hpp"

namespace other {

  class animation_pipeline : public asset_pipeline {
   public:
    animation_pipeline(event_system* events, asset_handler* handler)
        : asset_pipeline(events, handler) {}
    ~animation_pipeline() = default;
  };

}  // namespace other

#endif  // OTHER_SCENE_ASSET_PIPELINES_ANIMATION_PIPELINE_HPP
