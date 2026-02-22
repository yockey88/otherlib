/**
 * \file rendering-pipelines/scriptable_pipeline.hpp
 **/
#ifndef OTHERLIB_RENDERING_PIPELINES_SCRIPTABLE_PIPELINE_HPP
#define OTHERLIB_RENDERING_PIPELINES_SCRIPTABLE_PIPELINE_HPP

#include "renderer/render_pipeline.hpp"

namespace other {

  class scriptable_pipeline : public render_pipeline {
   public:
    virtual ~scriptable_pipeline() = default;

    void create_resources(renderer* renderer_ptr) override {}
    void build_render_passes(renderer* renderer_ptr) override {}
  };

}  // namespace other

#endif  // OTHERLIB_RENDERING_PIPELINES_SCRIPTABLE_PIPELINE_HPP