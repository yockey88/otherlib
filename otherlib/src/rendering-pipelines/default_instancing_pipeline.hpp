/**
 * \file rendering-pipelines/default_instancing_pipeline.hpp
 **/
#ifndef RENDERING_PIPELINES_DEFAULT_INSTANCING_PIPELINE_HPP
#define RENDERING_PIPELINES_DEFAULT_INSTANCING_PIPELINE_HPP

#include "renderer/render_pipeline.hpp"

namespace other {

  class default_instancing_pipeline : public render_pipeline {
   public:
    default_instancing_pipeline() = default;
    ~default_instancing_pipeline() = default;

   private:
    resource_handle quad_mesh_handle;

    resource_handle geometry_pass_shader_handle;
    resource_handle shadow_map_pass_shader_handle;
    resource_handle point_light_shadow_pass_shader_handle;
    resource_handle shading_pass_shader_handle;
    resource_handle debug_processing_shader_handle;
    resource_handle screen_shader_handle;

    void on_prepare_frame(renderer::frame_resources* resources, render_data* data) override;
    void create_resources() override;
    void build_render_passes() override;
  };

}  // namespace other

#endif  // RENDERING_PIPELINES_DEFAULT_INSTANCING_PIPELINE_HPP