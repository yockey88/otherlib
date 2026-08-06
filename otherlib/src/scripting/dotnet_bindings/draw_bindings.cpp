/**
 * \file scripting/dotnet_bindings/draw_bindings.cpp
 **/
#include "scripting/dotnet_bindings/draw_bindings.hpp"

#include "renderer/renderer.hpp"

#include "model/model_source.hpp"
#include "object/grid_component.hpp"
#include "object/render_component.hpp"
#include "scene/scene.hpp"

#include "driver/driver.hpp"
#include "scripting/dotnet_bindings/driver_bindings.hpp"
#include "scripting/dotnet_bindings/scene_bindings.hpp"

namespace other {
  namespace detail {

    static debug_draw get_draw_sink(nbool32 in_scene) {
      driver* drvr = detail::get_dotnet_native_driver();
      OTHER_ASSERT(drvr != nullptr, "Driver pointer is null in draw binding.");
      renderer& r = drvr->get_renderer();
      if (in_scene) {
        return r.scene_overlay();
      }
      /// drivers without a debug view never register the debug streams, drop those draws quietly
      if (r.get_stream_registry().find(builtin_debug_streams::kLines) == nullptr) {
        return debug_draw{ nullptr };
      }
      return r.debug();
    }

  }  // namespace detail
  namespace bindings {

    void native_draw_line(float ax, float ay, float az, float bx, float by, float bz, float r, float g, float b, float a, nbool32 in_scene) {
      debug_draw draw = detail::get_draw_sink(in_scene);
      draw.line({ ax, ay, az }, { bx, by, bz }, { r, g, b, a });
    }

    void native_draw_triangle(float ax, float ay, float az, float bx, float by, float bz, float cx, float cy, float cz, float r, float g, float b, float a, nbool32 in_scene) {
      debug_draw draw = detail::get_draw_sink(in_scene);
      draw.triangle({ ax, ay, az }, { bx, by, bz }, { cx, cy, cz }, { r, g, b, a });
    }

    void native_draw_point(float px, float py, float pz, float r, float g, float b, float a, nbool32 in_scene) {
      debug_draw draw = detail::get_draw_sink(in_scene);
      draw.point({ px, py, pz }, { r, g, b, a });
    }

    void native_draw_ray(float ox, float oy, float oz, float dx, float dy, float dz, float len, float r, float g, float b, float a, nbool32 in_scene) {
      debug_draw draw = detail::get_draw_sink(in_scene);
      draw.ray({ ox, oy, oz }, { dx, dy, dz }, len, { r, g, b, a });
    }

    void native_draw_arrow(float ax, float ay, float az, float bx, float by, float bz, float r, float g, float b, float a, nbool32 in_scene) {
      debug_draw draw = detail::get_draw_sink(in_scene);
      draw.arrow({ ax, ay, az }, { bx, by, bz }, { r, g, b, a });
    }

    void native_draw_aabb(float min_x, float min_y, float min_z, float max_x, float max_y, float max_z, float r, float g, float b, float a, nbool32 in_scene) {
      debug_draw draw = detail::get_draw_sink(in_scene);
      draw.aabb({ min_x, min_y, min_z }, { max_x, max_y, max_z }, { r, g, b, a });
    }

    void native_draw_obb(float m00, float m01, float m02, float m03,
                         float m10, float m11, float m12, float m13,
                         float m20, float m21, float m22, float m23,
                         float m30, float m31, float m32, float m33,
                         float r, float g, float b, float a,
                         nbool32 in_scene) {
      debug_draw draw = detail::get_draw_sink(in_scene);
      glm::mat4 xform{ { m00, m01, m02, m03 },
                       { m10, m11, m12, m13 },
                       { m20, m21, m22, m23 },
                       { m30, m31, m32, m33 } };
      draw.obb(xform, { r, g, b, a });
    }

    void native_draw_sphere(float cx, float cy, float cz, float radius, float r, float g, float b, float a, nbool32 in_scene) {
      debug_draw draw = detail::get_draw_sink(in_scene);
      draw.sphere({ cx, cy, cz }, radius, { r, g, b, a });
    }

    void native_draw_frustum(float m00, float m01, float m02, float m03,
                             float m10, float m11, float m12, float m13,
                             float m20, float m21, float m22, float m23,
                             float m30, float m31, float m32, float m33,
                             float r, float g, float b, float a,
                             nbool32 in_scene) {
      debug_draw draw = detail::get_draw_sink(in_scene);
      glm::mat4 inv_view_proj{ { m00, m01, m02, m03 },
                               { m10, m11, m12, m13 },
                               { m20, m21, m22, m23 },
                               { m30, m31, m32, m33 } };
      draw.frustum(inv_view_proj, { r, g, b, a });
    }

    void native_draw_transform(float m00, float m01, float m02, float m03,
                               float m10, float m11, float m12, float m13,
                               float m20, float m21, float m22, float m23,
                               float m30, float m31, float m32, float m33,
                               float scale, nbool32 in_scene) {
      debug_draw draw = detail::get_draw_sink(in_scene);
      glm::mat4 xform{ { m00, m01, m02, m03 },
                       { m10, m11, m12, m13 },
                       { m20, m21, m22, m23 },
                       { m30, m31, m32, m33 } };
      draw.transform(xform, scale);
    }

    void native_draw_mesh(uint64_t object_id, float m00, float m01, float m02, float m03,
                          float m10, float m11, float m12, float m13,
                          float m20, float m21, float m22, float m23,
                          float m30, float m31, float m32, float m33,
                          float r, float g, float b, float a,
                          nbool32 wireframe, nbool32 in_scene) {
      scene* active_scene = detail::get_active_scene_checked();
      OTHER_ASSERT(active_scene->has_object(object_id), "Draw.Mesh: object {} does not exist in the active scene.", object_id);

      const render_component* rc = active_scene->try_get_component<render_component>(object_id);
      if (rc == nullptr || rc->obj_model.source == nullptr || !rc->obj_model.source->uploaded()) {
        /// nothing renderable yet (no component, or the async model load hasn't landed)
        return;
      }

      debug_draw draw = detail::get_draw_sink(in_scene);
      glm::mat4 model{ { m00, m01, m02, m03 },
                       { m10, m11, m12, m13 },
                       { m20, m21, m22, m23 },
                       { m30, m31, m32, m33 } };
      draw.mesh(rc->obj_model.source->get_mesh_handle(), model, { r, g, b, a }, wireframe);
    }

    void native_draw_grid(uint64_t object_id, nbool32 in_scene) {
      scene* active_scene = detail::get_active_scene_checked();
      OTHER_ASSERT(active_scene->has_object(object_id), "Draw.Grid: object {} does not exist in the active scene.", object_id);

      const grid_component* grid = active_scene->try_get_component<grid_component>(object_id);
      OTHER_ASSERT(grid != nullptr, "Draw.Grid: object {} has no grid component.", object_id);

      if (grid->extent == 0 || grid->cell_size <= 0.f) {
        return;
      }

      const glm::mat4 world = active_scene->get_world_transform(object_id);

      driver* drvr = detail::get_dotnet_native_driver();
      OTHER_ASSERT(drvr != nullptr, "Driver pointer is null in draw binding.");
      renderer& r = drvr->get_renderer();

      /// spherical grids have no procedural pass yet, their line shell rides the overlay streams
      if (in_scene && grid->coordinate_system != GRID_COORDINATES_SPHERICAL) {
        /// cylindrical grids stack one procedural plane per layer
        const uint32_t layer_count = grid_draw_layer_count(*grid);
        for (uint32_t layer = 0; layer < layer_count; ++layer) {
          r.submit_grid(make_grid_draw_data(*grid, world, layer));
        }
        if (grid->coordinate_system == GRID_COORDINATES_CYLINDRICAL) {
          /// the quads cannot draw the verticals between layers, those ride the scene line stream
          debug_draw draw = detail::get_draw_sink(in_scene);
          emit_grid_shell_lines(draw, *grid, world);
        }
        return;
      }

      debug_draw draw = detail::get_draw_sink(in_scene);
      emit_grid_lines(draw, *grid, world);
    }

  }  // namespace bindings
}  // namespace other
