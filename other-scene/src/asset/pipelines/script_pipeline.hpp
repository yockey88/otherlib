/**
 * \file asset/pipelines/script_pipeline.hpp
 **/
#ifndef OTHER_SCENE_ASSET_PIPELINES_SCRIPT_PIPELINE_HPP
#define OTHER_SCENE_ASSET_PIPELINES_SCRIPT_PIPELINE_HPP

#include "asset/asset_pipeline.hpp"

namespace other {

  class script_pipeline : public asset_pipeline {
   public:
    script_pipeline(event_system& events, asset_handler* handler)
        : asset_pipeline(events, handler) {}
    ~script_pipeline() = default;
  };

}  // namespace other

#endif  // OTHER_SCENE_ASSET_PIPELINES_SCRIPT_PIPELINE_HPP