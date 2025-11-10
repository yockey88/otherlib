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

    void create_resources() override {}
    void build_render_passes() override {}
  };

}  // namespace other

#endif  // OTHERLIB_RENDERING_PIPELINES_SCRIPTABLE_PIPELINE_HPP