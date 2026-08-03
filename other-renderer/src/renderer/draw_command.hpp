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

  enum stencil_func {
    STENCIL_NEVER = 0,
    STENCIL_LESS = 1,
    STENCIL_EQUAL = 2,
    STENCIL_LEQUAL = 3,
    STENCIL_GREATER = 4,
    STENCIL_NOTEQUAL = 5,
    STENCIL_GEQUAL = 6,
    STENCIL_ALWAYS = 7
  };

  enum stencil_op {
    STENCIL_KEEP = 0,
    STENCIL_ZERO = 1,
    STENCIL_REPLACE = 2,
    STENCIL_INCR = 3,
    STENCIL_INCR_WRAP = 4,
    STENCIL_DECR = 5,
    STENCIL_DECR_WRAP = 6,
    STENCIL_INVERT = 7
  };

  enum depth_func {
    DEPTH_NEVER = 0,
    DEPTH_LESS = 1,
    DEPTH_EQUAL = 2,
    DEPTH_LEQUAL = 3,
    DEPTH_GREATER = 4,
    DEPTH_NOTEQUAL = 5,
    DEPTH_GEQUAL = 6,
    DEPTH_ALWAYS = 7
  };

  struct mesh_key {
    resource_handle model_source_handle = {};
    render_polygon_mode render_state = render_polygon_mode::POLYGON_MODE_FILL;
    mesh::primitive_type draw_mode = mesh::primitive_type::TRIANGLES;

    uint32_t submesh_index = 0;

    /// material identity joins the batch key because texture binds are per draw call:
    ///  component override material path_hash, or 0 = the model's imported material
    natural_t material_key = 0;

    constexpr auto operator<=>(const mesh_key&) const = default;
  };

  /// which half of the frame's draw-list partition a scene pass consumes: opaque draws feed
  ///  the gbuffer/shadow/voxelize passes, transparent draws only the blended forward pass.
  ///  (k-prefixed because wingdi.h claims OPAQUE and TRANSPARENT as macros)
  enum class draw_set : uint8_t {
    kOpaque,
    kTransparent,
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
  field(line_thickness, other::attr::serializable()))

#endif  // OTHER_RENDERER_RENDERER_RAW_COMMAND_HPP