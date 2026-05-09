/**
 * \file asset/pipelines/script_project_pipeline.hpp
 **/
#ifndef OTHER_SCENE_ASSET_PIPELINES_SCRIPT_PROJECT_PIPELINE_HPP
#define OTHER_SCENE_ASSET_PIPELINES_SCRIPT_PROJECT_PIPELINE_HPP

#include "asset/asset_pipeline.hpp"

namespace other {

  class script_project_pipeline : public asset_pipeline {
   public:
    script_project_pipeline(event_system& events, asset_handler* handler)
        : asset_pipeline(events, handler) {}
    virtual ~script_project_pipeline() = default;

    filepath csproj_path;
  };

}  // namespace other

#endif  // OTHER_SCENE_ASSET_PIPELINES_SCRIPT_PROJECT_PIPELINE_HPP