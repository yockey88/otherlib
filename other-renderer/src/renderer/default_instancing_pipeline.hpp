/**
 * \file renderer/default_instancing_pipeline.hpp
 **/
#ifndef OTHER_RENDERER_DEFAULT_INSTANCING_PIPELINE_HPP
#define OTHER_RENDERER_DEFAULT_INSTANCING_PIPELINE_HPP

#include "renderer/render_pipeline.hpp"

namespace other {

  class default_instancing_pipeline : public render_pipeline {
   public:
    default_instancing_pipeline() = default;
    ~default_instancing_pipeline() = default;

   private:
    resource_handle quad_mesh_handle;

    resource_handle instancing_shader;
    resource_handle screen_shader_handle;

    void create_resources() override;
    void build_render_passes() override;

    void prepare_frame(render_data* data) override;
  };

}  // namespace other

#endif  // OTHER_RENDERER_DEFAULT_INSTANCING_PIPELINE_HPP