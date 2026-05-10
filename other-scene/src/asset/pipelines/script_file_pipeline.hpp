/**
 * \file asset/pipelines/script_file_pipeline.hpp
 **/
#ifndef OTHER_SCENE_ASSET_PIPELINES_SCRIPT_FILE_PIPELINE_HPP
#define OTHER_SCENE_ASSET_PIPELINES_SCRIPT_FILE_PIPELINE_HPP

#include "asset/asset_pipeline.hpp"

namespace other {

  class script_file_pipeline : public asset_pipeline {
   public:
    script_file_pipeline(event_system& events, asset_handler* handler)
        : asset_pipeline(events, handler) {}
    ~script_file_pipeline() = default;

    filepath script_path;
  };

}  // namespace other

#endif  // OTHER_SCENE_ASSET_PIPELINES_SCRIPT_FILE_PIPELINE_HPP