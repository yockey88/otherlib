/**
 * \file ui/viewport/editor_viewport_pipelines.cpp
 **/
#include "ui/viewport/editor_viewport_pipelines.hpp"

namespace other {

  pipeline_definition create_outline_selection_effect_pipeline() {
    return {
      .name = "outline-selection-effect",
      .textures = {
        {
          .name = "viewport-stencil",
          .format = texture::format::R8,
        },
      },
    };
  }

}  // namespace other