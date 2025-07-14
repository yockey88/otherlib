/**
 * \file renderer/pipelines/default_instancing_pipeline.hpp
 **/
#ifndef OTHER_RENDERER_PIPELINES_DEFAULT_INSTANCING_PIPELINE_HPP
#define OTHER_RENDERER_PIPELINES_DEFAULT_INSTANCING_PIPELINE_HPP

#include "renderer/render_pipeline.hpp"

namespace other {

  class default_instancing_pipeline : public render_pipeline {
   public:
    default_instancing_pipeline() = default;
    ~default_instancing_pipeline() = default;

    void prepare_frame(renderer::frame_resources* resources, render_data* data) override;

   private:
    resource_handle quad_mesh_handle;

    resource_handle geometry_pass_shader_handle;
    resource_handle shading_pass_shader_handle;
    resource_handle screen_shader_handle;

    void create_resources() override;
    void build_render_passes() override;
  };

}  // namespace other

#endif  // OTHER_RENDERER_PIPELINES_DEFAULT_INSTANCING_PIPELINE_HPP