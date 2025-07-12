/**
 * \file renderer/draw_command.cpp
 **/
#include "renderer/draw_command.hpp"

#include "gpu_resource/shader.hpp"
#include "model/model.hpp"

namespace other {

  draw_command::operator mesh_key() const {
    OTHER_ASSERT(draw_model != nullptr, "Draw command must have a valid model.");
    OTHER_ASSERT(draw_model->source != nullptr, "Draw command model source must not be null.");

    return {
      .model_source_handle = draw_model->source->get_mesh_handle(),
      .render_state = render_state,
      .draw_mode = draw_mode,
      .submesh_index = submesh_index,
    };
  }

}  // namespace other