/**
 * \file rendering_pipelines/empty_pipeline.hpp
 **/
#ifndef OTHERLIB_RENDERING_PIPELINES_EMPTY_PIPELINE_HPP
#define OTHERLIB_RENDERING_PIPELINES_EMPTY_PIPELINE_HPP

#include "renderer/render_pipeline.hpp"

namespace other {

  class empty_pipeline : public render_pipeline {
   public:
    virtual ~empty_pipeline() override = default;

    void create_resources() override;
    void build_render_passes() override;
  };

}  // namespace other

#endif  // OTHERLIB_RENDERING_PIPELINES_EMPTY_PIPELINE_HPP