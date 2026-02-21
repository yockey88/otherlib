/**
 * \file rendering_pipelines/empty_pipeline.cpp
 **/
#include "rendering-pipelines/empty_pipeline.hpp"

namespace other {

  void empty_pipeline::create_resources(renderer* renderer_ptr) {
    add_buffer_resource("camera_buffer", gpu_buffer::buf_type::UNIFORM_BUFFER, gpu_buffer::usage::DYNAMIC);
    add_buffer_resource("point_light_buffer", gpu_buffer::buf_type::STORAGE_BUFFER, gpu_buffer::usage::DYNAMIC);
    add_buffer_resource("bone_buffer", gpu_buffer::buf_type::STORAGE_BUFFER, gpu_buffer::usage::DYNAMIC);
    add_buffer_resource("direction_light_buffer", gpu_buffer::buf_type::STORAGE_BUFFER, gpu_buffer::usage::DYNAMIC);
    add_buffer_resource("light_matrix_buffer", gpu_buffer::buf_type::STORAGE_BUFFER, gpu_buffer::usage::DYNAMIC);
    add_buffer_resource("material_buffer", gpu_buffer::buf_type::STORAGE_BUFFER, gpu_buffer::usage::DYNAMIC);
    add_buffer_resource("model_buffer", gpu_buffer::buf_type::UNIFORM_BUFFER, gpu_buffer::usage::DYNAMIC);

    set_model_buffer("model_buffer");
    set_material_buffer("material_buffer");
    set_bone_buffer("bone_buffer");
    set_point_light_buffer("point_light_buffer");
    set_direction_light_buffer("direction_light_buffer");
    set_camera_buffer("camera_buffer");
  }

  void empty_pipeline::build_render_passes(renderer* renderer_ptr) {
  }

}  // namespace other