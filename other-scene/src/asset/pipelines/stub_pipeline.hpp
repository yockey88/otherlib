/**
 * \file asset/pipelines/stub_pipeline.hpp
 *
 * pipeline for asset types whose runtime backend does not exist yet (audio,
 * animation, input-map): the asset is tracked, watched, and resolvable, but the
 * loader carries no payload. replace per-type when the backend lands.
 **/
#ifndef OTHER_SCENE_ASSET_PIPELINES_STUB_PIPELINE_HPP
#define OTHER_SCENE_ASSET_PIPELINES_STUB_PIPELINE_HPP

#include "asset/asset_pipeline.hpp"

namespace other {

  class stub_pipeline : public asset_pipeline {
   public:
    stub_pipeline(event_system* events, asset_handler* handler)
        : asset_pipeline(events, handler) {}
    ~stub_pipeline() = default;
  };

}  // namespace other

#endif  // OTHER_SCENE_ASSET_PIPELINES_STUB_PIPELINE_HPP
