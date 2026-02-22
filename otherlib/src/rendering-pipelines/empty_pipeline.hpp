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

    void create_resources(renderer* renderer_ptr) override;
    void build_render_passes(renderer* renderer_ptr) override;
  };

}  // namespace other

#endif  // OTHERLIB_RENDERING_PIPELINES_EMPTY_PIPELINE_HPP