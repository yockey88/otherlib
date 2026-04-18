/**
 * \file asset/pipelines/rendering_pipeline_pipeline.hpp
 **/
#ifndef OTHER_SCENE_ASSET_PIPELINES_RENDERING_PIPELINE_PIPELINE_HPP
#define OTHER_SCENE_ASSET_PIPELINES_RENDERING_PIPELINE_PIPELINE_HPP

#include "renderer/render_pipeline.hpp"

#include "asset/asset_pipeline.hpp"

namespace other {

  class rendering_pipeline_pipeline : public asset_pipeline {
   public:
    rendering_pipeline_pipeline(event_system& events, asset_handler* handler)
        : asset_pipeline(events, handler) {}
    rendering_pipeline_pipeline(event_system& events, asset_handler* handler, const pipeline_definition& definition)
        : asset_pipeline(events, handler), definition(definition) {}
    ~rendering_pipeline_pipeline() override = default;

    pipeline_definition definition;

   private:
    void on_load_complete(asset* asset_ptr) override;
    void on_load_failed(asset* asset_ptr, const std::string& error_message) override;

    void on_unload_complete(asset* asset_ptr) override;
    void on_unload_failed(asset* asset_ptr, const std::string& error_message) override;

    void on_pipeline_poll() override;
  };

}  // namespace other

#endif  // OTHER_SCENE_ASSET_PIPELINES_RENDERING_PIPELINE_PIPELINE_HPP