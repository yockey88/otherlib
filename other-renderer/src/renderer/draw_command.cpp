/**
 * \file renderer/draw_command.cpp
 **/
#include "renderer/draw_command.hpp"

#include "model/model.hpp"

namespace other {

  draw_command::operator mesh_key() const {
    return {
      .model_source_handle = draw_model->source->get_mesh_handle(),
      .render_state = render_state,
      .draw_mode = draw_mode,
      .line_thickness = line_thickness,
      .submesh_index = submesh_index,
    };
  }

}  // namespace other