/**
 * \file rendering_pipelines/empty_pipeline.hpp
 **/
#ifndef RENDERING_PIPELINES_EMPTY_PIPELINE_HPP
#define RENDERING_PIPELINES_EMPTY_PIPELINE_HPP

#include "renderer/render_pipeline.hpp"

namespace other {

  class empty_pipeline : public render_pipeline {
   public:
    void create_resources() override;
    void build_render_passes() override;
  };

}  // namespace other

#endif  // RENDERING_PIPELINES_EMPTY_PIPELINE_HPP