/**
 * \file renderer/draw_command.hpp
 **/
#ifndef OTHER_RENDERER_RENDERER_RAW_COMMAND_HPP
#define OTHER_RENDERER_RENDERER_RAW_COMMAND_HPP

#include <cstdint>

#include "core/arena_buffer.hpp"
#include "core/defines.hpp"

#include "gpu_resource/mesh.hpp"
#include "gpu_resource/renderer_resource.hpp"
#include "renderer/gpu_structs.hpp"

namespace other {

  struct mesh;
  struct model;
  struct shader;

  enum render_polygon_mode {
    POLYGON_MODE_FILL = 0,
    POLYGON_MODE_LINE,
    POLYGON_MODE_POINT
  };

  struct mesh_key {
    resource_handle model_source_handle = {};
    render_polygon_mode render_state = render_polygon_mode::POLYGON_MODE_FILL;
    mesh::primitive_type draw_mode = mesh::primitive_type::TRIANGLES;

    uint32_t submesh_index = 0;

    constexpr auto operator<=>(const mesh_key&) const = default;
  };

  struct draw_call {
    resource_handle mesh_handle;
    // resource_handle bone_buffer_handle;

    uint32_t submesh_index = 0;

    uint32_t base_instance = 0;
    uint32_t instance_count = 0;

    uint32_t vertex_offset = 0;
    uint32_t vertex_count = 0;

    uint32_t index_offset = 0;
    uint32_t index_count = 0;

    float line_thickness = 1.f;
  };

  /*
  struct DrawElementsIndirectCommand {
    uint32_t  count;
    uint32_t  instanceCount;
    uint32_t  firstIndex;
    int32_t  baseVertex;
    uint32_t  baseInstance;
  };
  */

}  // namespace other

OTHER_REFLECT(
  other::draw_call,
  field(submesh_index, other::attr::serializable()),
  field(base_instance, other::attr::serializable()),
  field(instance_count, other::attr::serializable()),
  field(vertex_offset, other::attr::serializable()),
  field(vertex_count, other::attr::serializable()),
  field(index_offset, other::attr::serializable()),
  field(index_count, other::attr::serializable()),
  field(line_thickness, other::attr::serializable())
)

#endif  // OTHER_RENDERER_RENDERER_RAW_COMMAND_HPP