/**
 * \file asset/pipelines/asset_declaration_pipeline.hpp
 **/
#ifndef OTHER_SCENE_ASSET_PIPELINES_ASSET_DECLARATION_PIPELINE_HPP
#define OTHER_SCENE_ASSET_PIPELINES_ASSET_DECLARATION_PIPELINE_HPP

#include "asset/asset_pipeline.hpp"

namespace other {

  class asset_declaration_pipeline : public asset_pipeline {
   public:
    asset_declaration_pipeline(event_system* events, asset_handler* handler)
        : asset_pipeline(events, handler) {}
    ~asset_declaration_pipeline() override {}
  };

}  // namespace other

#endif  // OTHER_SCENE_ASSET_PIPELINES_ASSET_DECLARATION_PIPELINE_HPP