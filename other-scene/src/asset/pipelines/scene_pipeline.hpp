/**
 * \file asset/pipelines/scene_pipeline.hpp
 **/
#ifndef OTHER_SCENE_ASSET_PIPELINES_SCENE_PIPELINE_HPP
#define OTHER_SCENE_ASSET_PIPELINES_SCENE_PIPELINE_HPP

#include "asset/asset_pipeline.hpp"

namespace other {

  class scene_pipeline : public asset_pipeline {
   public:
    scene_pipeline(event_system& events, asset_handler* handler, scene* scene_ptr)
        : asset_pipeline(events, handler), scene_ptr(scene_ptr) {}
    ~scene_pipeline() override = default;

    scene* scene_ptr = nullptr;

   private:
    void on_load_complete(asset* asset_ptr) override;
    void on_load_failed(asset* asset_ptr, const std::string& error_message) override;

    void on_unload_complete(asset* asset_ptr) override;
    void on_unload_failed(asset* asset_ptr, const std::string& error_message) override;

    void on_pipeline_poll() override;
  };

}  // namespace other

#endif  // OTHER_SCENE_ASSET_PIPELINES_SCENE_PIPELINE_HPP