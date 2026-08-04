/**
 * \file asset/pipelines/audio_pipeline.hpp
 **/
#ifndef OTHER_SCENE_ASSET_PIPELINES_AUDIO_PIPELINE_HPP
#define OTHER_SCENE_ASSET_PIPELINES_AUDIO_PIPELINE_HPP

#include "asset/asset_pipeline.hpp"

namespace other {

  class audio_pipeline : public asset_pipeline {
   public:
    audio_pipeline(event_system* events, asset_handler* handler)
        : asset_pipeline(events, handler) {}
    ~audio_pipeline() = default;
  };

}  // namespace other

#endif  // OTHER_SCENE_ASSET_PIPELINES_AUDIO_PIPELINE_HPP
