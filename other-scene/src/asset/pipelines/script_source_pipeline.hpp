/**
 * \file asset/pipelines/script_source_pipeline.hpp
 **/
#ifndef OTHER_SCENE_ASSET_PIPELINES_SCRIPT_SOURCE_PIPELINE_HPP
#define OTHER_SCENE_ASSET_PIPELINES_SCRIPT_SOURCE_PIPELINE_HPP

#include "asset/asset_pipeline.hpp"

namespace other {

  class script_source_pipeline : public asset_pipeline {
   public:
    script_source_pipeline(event_system& events, asset_handler* handler)
        : asset_pipeline(events, handler) {}
    ~script_source_pipeline() override = default;

    filepath script_path;
  };

}  // namespace other

#endif  // OTHER_SCENE_ASSET_PIPELINES_SCRIPT_SOURCE_PIPELINE_HPP