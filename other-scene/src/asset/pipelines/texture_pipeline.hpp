/**
 * \file asset/pipelines/texture_pipeline.hpp
 **/
#ifndef OTHER_SCENE_ASSET_PIPELINES_TEXTURE_PIPELINE_HPP
#define OTHER_SCENE_ASSET_PIPELINES_TEXTURE_PIPELINE_HPP

#include "asset/asset_pipeline.hpp"

namespace other {

  class texture_pipeline : public asset_pipeline {
   public:
    texture_pipeline(event_system* events, asset_handler* handler)
        : asset_pipeline(events, handler) {}
    ~texture_pipeline() = default;
  };

}  // namespace other

#endif  // OTHER_SCENE_ASSET_PIPELINES_TEXTURE_PIPELINE_HPP
