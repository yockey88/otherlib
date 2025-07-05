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
    resource_handle model_source_handle;
    render_polygon_mode render_state;
    mesh::primitive_type draw_mode;

    uint32_t submesh_index = 0;

    constexpr auto operator<=>(const mesh_key&) const = default;
  };

  /**
   * renderer-frontend takes draw_commands and turns them into a list of mesh keys and draw calls
   * renderer-backend takes a render-graph with resources and the map of mesh keys to draw calls and uses the render-graph to
   *                   to execute the draw calls
   **/

  struct draw_command {
    model* draw_model = nullptr;
    shader* shader_handle = nullptr;
    glm::mat4 transform = glm::mat4(1.f);

    gpu::graphics_material material = {};

    uint32_t submesh_index = 0;
    render_polygon_mode render_state = render_polygon_mode::POLYGON_MODE_FILL;
    mesh::primitive_type draw_mode = mesh::primitive_type::TRIANGLES;

    real_t line_thickness = 1.f;

    operator mesh_key() const;
  };

  struct draw_call {
    mesh* mesh = nullptr;
    shader* shader = nullptr;
    uint32_t submesh_index = 0;

    uint32_t base_instance = 0;
    uint32_t instance_count = 0;

    uint32_t vertex_offset = 0;
    uint32_t vertex_count = 0;

    uint32_t index_offset = 0;
    uint32_t index_count = 0;

    float line_thickness = 1.f;
  };

  // struct SubMeshDrawCall {
  //   arena_buffer cpu_model_storage;
  //   arena_buffer cpu_material_storage;

  //   uint32_t vertex_offset = 0;
  //   uint32_t vertex_count = 0;

  //   uint32_t index_offset = 0;
  //   uint32_t index_count = 0;

  //   uint32_t instance_count = 0;
  // };

  // struct MeshDrawCall {
  //   // Ref<VertexArray> vao = nullptr;
  //   uint32_t base_instance = 0;

  //   // Ref<MaterialTable> material_table = nullptr;
  //   std::vector<SubMeshDrawCall> submissions;

  //   float line_thickness = 1.f;
  // };

  /**

      model_storage->BindBase();
      model_storage->LoadFromBuffer(sub_call.cpu_model_storage);
      material_storage->BindBase();
      material_storage->LoadFromBuffer(sub_call.cpu_material_storage);

      glLineWidth(draw_call.line_thickness);
      glPolygonMode(GL_FRONT_AND_BACK, mesh_key.render_state);
      glDrawElementsInstancedBaseVertexBaseInstance(mesh_key.draw_mode, sub_call.index_count, GL_UNSIGNED_INT, (void*)0, sub_call.instance_count, sub_call.vertex_offset, 0);
   */

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_RAW_COMMAND_HPP