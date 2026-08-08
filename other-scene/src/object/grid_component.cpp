/**
 * \file object/grid_component.cpp
 **/
#include "object/grid_component.hpp"

#include <algorithm>

#include <glm/gtc/constants.hpp>

#include "core/profiler.hpp"

namespace other {
  namespace {

    constexpr uint32_t kRingSegments = 64;
    /// caps keep a single grid comfortably inside the debug line stream budget
    constexpr uint32_t kMaxCartesianExtent = 2048;
    constexpr uint32_t kMaxRingExtent = 256;
    constexpr uint32_t kMaxLayerExtent = 16;
    /// cylindrical off-plane layers thin rings to the major cadence + boundary ring past this
    ///   circle count, capping the stack at one extra maxed-out polar grid worth of lines
    constexpr uint32_t kMaxOffPlaneRingCircles = 256;
    /// cylindrical shell verticals thin the same way, on the same emitted-line budget
    constexpr uint32_t kMaxShellVerticalLines = kMaxOffPlaneRingCircles * kRingSegments;

    /// layers stacked on each side of the base plane after the caps (0 unless cylindrical)
    uint32_t clamped_layer_extent(const grid_component& grid) {
      if (grid.coordinate_system != GRID_COORDINATES_CYLINDRICAL || grid.layer_spacing <= 0.f) {
        return 0;
      }
      return std::min(grid.layer_extent, kMaxLayerExtent);
    }

  }  // namespace

  grid_basis grid_basis_for_plane(uint32_t plane) {
    switch (plane) {
      case GRID_PLANE_XY: return { { 1.f, 0.f, 0.f }, { 0.f, 1.f, 0.f }, { 0.f, 0.f, 1.f } };
      case GRID_PLANE_YZ: return { { 0.f, 1.f, 0.f }, { 0.f, 0.f, 1.f }, { 1.f, 0.f, 0.f } };
      case GRID_PLANE_XZ:
      default:            return { { 1.f, 0.f, 0.f }, { 0.f, 0.f, 1.f }, { 0.f, 1.f, 0.f } };
    }
  }

  glm::vec4 grid_axis_color(const glm::vec3& local_axis) {
    if (glm::abs(local_axis.x) > 0.5f) {
      return basic_colors::kRed;
    }
    if (glm::abs(local_axis.y) > 0.5f) {
      return basic_colors::kGreen;
    }
    return basic_colors::kBlue;
  }

  uint32_t grid_draw_layer_count(const grid_component& grid) {
    return 2 * clamped_layer_extent(grid) + 1;
  }

  grid_draw_data make_grid_draw_data(const grid_component& grid, const glm::mat4& world, uint32_t layer) {
    OTHER_ASSERT(grid.coordinate_system < NUM_GRID_COORDINATE_SYSTEMS, "Grid component has invalid coordinate system: {}", grid.coordinate_system);
    OTHER_ASSERT(grid.plane < NUM_GRID_PLANES, "Grid component has invalid plane: {}", grid.plane);
    OTHER_ASSERT(layer < grid_draw_layer_count(grid), "Grid layer {} out of range for {} layers", layer, grid_draw_layer_count(grid));

    const auto [u, v, n] = grid_basis_for_plane(grid.plane);
    const int32_t layer_offset = static_cast<int32_t>(layer) - static_cast<int32_t>(clamped_layer_extent(grid));

    grid_draw_data gd;
    gd.basis_u = glm::vec4(glm::normalize(glm::vec3(world * glm::vec4(u, 0.f))), 0.f);
    gd.basis_v = glm::vec4(glm::normalize(glm::vec3(world * glm::vec4(v, 0.f))), 0.f);
    gd.origin = world * glm::vec4(grid.origin, 1.f);
    if (layer_offset != 0) {
      const glm::vec3 world_n = glm::normalize(glm::vec3(world * glm::vec4(n, 0.f)));
      gd.origin += glm::vec4(world_n * (grid.layer_spacing * static_cast<float>(layer_offset)), 0.f);
    }
    gd.line_color = grid.line_color;
    gd.major_line_color = grid.major_line_color;
    gd.axis_u_color = grid_axis_color(u);
    gd.axis_v_color = grid_axis_color(v);
    gd.cell_size = grid.cell_size;
    gd.line_width = grid.line_width;
    gd.extent = grid.extent;
    gd.major_line_every = grid.major_line_every;
    gd.sector_count = grid.sector_count;
    gd.polar = grid.coordinate_system == GRID_COORDINATES_POLAR || grid.coordinate_system == GRID_COORDINATES_CYLINDRICAL;
    gd.show_axes = grid.show_axes && layer_offset == 0;
    return gd;
  }

  void emit_grid_lines(debug_draw& draw, const grid_component& grid, const glm::mat4& world) {
    OTHER_ASSERT(grid.coordinate_system < NUM_GRID_COORDINATE_SYSTEMS, "Grid component has invalid coordinate system: {}", grid.coordinate_system);
    OTHER_ASSERT(grid.plane < NUM_GRID_PLANES, "Grid component has invalid plane: {}", grid.plane);

    if (grid.cell_size <= 0.f || grid.extent == 0) {
      return;
    }
    PROFILE_SECTION("emit_grid_lines");

    const auto [u, v, n] = grid_basis_for_plane(grid.plane);
    const auto tp = [&](const glm::vec3& p) { return glm::vec3(world * glm::vec4(grid.origin + p, 1.f)); };
    const auto is_major = [&](uint32_t i) { return grid.major_line_every > 0 && i % grid.major_line_every == 0; };
    const auto line_col = [&](uint32_t i) { return is_major(i) ? grid.major_line_color : grid.line_color; };

    if (grid.coordinate_system == GRID_COORDINATES_CARTESIAN) {
      const uint32_t extent = std::min(grid.extent, kMaxCartesianExtent);
      const float half = static_cast<float>(extent) * grid.cell_size;
      for (uint32_t i = 0; i <= 2 * extent; ++i) {
        const float d = static_cast<float>(i) * grid.cell_size - half;
        const bool on_axis = i == extent;

        /// line parallel to u at offset d along v, then parallel to v at offset d along u
        const glm::vec4 cu = on_axis && grid.show_axes ? grid_axis_color(u) : line_col(i > extent ? i - extent : extent - i);
        const glm::vec4 cv = on_axis && grid.show_axes ? grid_axis_color(v) : line_col(i > extent ? i - extent : extent - i);
        draw.line(tp(v * d - u * half), tp(v * d + u * half), cu);
        draw.line(tp(u * d - v * half), tp(u * d + v * half), cv);
      }
      return;
    }

    /// polar, spherical, and cylindrical share the in-plane ring/spoke layout
    const uint32_t rings = std::min(grid.extent, kMaxRingExtent);
    const float outer_radius = static_cast<float>(rings) * grid.cell_size;
    constexpr float tau = 2.f * glm::pi<float>();

    const auto ring_point = [&](float radius, float theta) { return u * (radius * glm::cos(theta)) + v * (radius * glm::sin(theta)); };
    const auto ring_circle = [&](float radius, const glm::vec3& lift, const glm::vec4& color) {
      for (uint32_t seg = 0; seg < kRingSegments; ++seg) {
        const float t0 = static_cast<float>(seg) / kRingSegments * tau;
        const float t1 = static_cast<float>(seg + 1) / kRingSegments * tau;
        draw.line(tp(ring_point(radius, t0) + lift), tp(ring_point(radius, t1) + lift), color);
      }
    };

    for (uint32_t ring = 1; ring <= rings; ++ring) {
      ring_circle(static_cast<float>(ring) * grid.cell_size, glm::vec3(0.f), line_col(ring));
    }

    for (uint32_t sector = 0; sector < grid.sector_count; ++sector) {
      const float theta = static_cast<float>(sector) / static_cast<float>(grid.sector_count) * tau;
      const bool is_u_axis = sector == 0;
      const glm::vec4 c = is_u_axis && grid.show_axes ? grid_axis_color(u) : grid.line_color;
      draw.line(tp(glm::vec3(0.f)), tp(ring_point(outer_radius, theta)), c);
    }

    if (grid.coordinate_system == GRID_COORDINATES_CYLINDRICAL) {
      PROFILE_SECTION("emit_grid_lines--cylindrical_layers");
      /// off-plane layers repeat the full polar pattern (lattice reads above/below the base plane),
      ///   thinning rings to the major cadence then the boundary ring once the stack outgrows budget
      const uint32_t layers = clamped_layer_extent(grid);
      uint32_t ring_step = 1;
      if (2u * layers * rings > kMaxOffPlaneRingCircles && grid.major_line_every > 0) {
        ring_step = grid.major_line_every;
      }
      const bool boundary_only = 2u * layers * (rings / ring_step) > kMaxOffPlaneRingCircles;
      for (int32_t layer = -static_cast<int32_t>(layers); layer <= static_cast<int32_t>(layers); ++layer) {
        if (layer == 0) {
          continue;
        }
        const glm::vec3 lift = n * (grid.layer_spacing * static_cast<float>(layer));
        if (!boundary_only) {
          for (uint32_t ring = ring_step; ring <= rings; ring += ring_step) {
            ring_circle(static_cast<float>(ring) * grid.cell_size, lift, line_col(ring));
          }
        }
        /// the boundary ring always closes the layer so the shell verticals land on something
        if (boundary_only || rings % ring_step != 0) {
          ring_circle(outer_radius, lift, line_col(rings));
        }
        for (uint32_t sector = 0; sector < grid.sector_count; ++sector) {
          const float theta = static_cast<float>(sector) / static_cast<float>(grid.sector_count) * tau;
          draw.line(tp(lift), tp(ring_point(outer_radius, theta) + lift), grid.line_color);
        }
      }
      emit_grid_shell_lines(draw, grid, world);
    }

    if (grid.coordinate_system == GRID_COORDINATES_SPHERICAL) {
      PROFILE_SECTION("emit_grid_lines--spherical_meridians");
      /// meridian great circles through the poles give the outer shell its sphere skeleton
      const auto meridian_point = [&](float theta, float phi) {
        const glm::vec3 dir = u * glm::cos(theta) + v * glm::sin(theta);
        return dir * (outer_radius * glm::cos(phi)) + n * (outer_radius * glm::sin(phi));
      };
      const uint32_t meridians = std::max(grid.sector_count / 2, 1u);
      for (uint32_t m = 0; m < meridians; ++m) {
        const float theta = static_cast<float>(m) / static_cast<float>(meridians) * glm::pi<float>();
        for (uint32_t seg = 0; seg < kRingSegments; ++seg) {
          const float p0 = static_cast<float>(seg) / kRingSegments * tau;
          const float p1 = static_cast<float>(seg + 1) / kRingSegments * tau;
          draw.line(tp(meridian_point(theta, p0)), tp(meridian_point(theta, p1)), grid.line_color);
        }
      }

      if (grid.show_axes) {
        draw.line(tp(-n * outer_radius), tp(n * outer_radius), grid_axis_color(n));
      }
    }
  }

  void emit_grid_shell_lines(debug_draw& draw, const grid_component& grid, const glm::mat4& world) {
    OTHER_ASSERT(grid.plane < NUM_GRID_PLANES, "Grid component has invalid plane: {}", grid.plane);

    const uint32_t layers = clamped_layer_extent(grid);
    if (layers == 0 || grid.cell_size <= 0.f || grid.extent == 0) {
      return;
    }
    PROFILE_SECTION("emit_grid_shell_lines");

    const auto [u, v, n] = grid_basis_for_plane(grid.plane);
    const auto tp = [&](const glm::vec3& p) { return glm::vec3(world * glm::vec4(grid.origin + p, 1.f)); };

    const uint32_t rings = std::min(grid.extent, kMaxRingExtent);
    const float outer_radius = static_cast<float>(rings) * grid.cell_size;
    const float half_height = static_cast<float>(layers) * grid.layer_spacing;
    constexpr float tau = 2.f * glm::pi<float>();

    const auto is_major = [&](uint32_t i) { return grid.major_line_every > 0 && i % grid.major_line_every == 0; };
    const auto line_col = [&](uint32_t i) { return is_major(i) ? grid.major_line_color : grid.line_color; };

    /// verticals at each ring/sector intersection tie the stacked layers into one lattice, thinning
    ///   to the major cadence and finally the boundary ring once they outgrow the budget
    uint32_t ring_step = 1;
    if (rings * grid.sector_count > kMaxShellVerticalLines && grid.major_line_every > 0) {
      ring_step = grid.major_line_every;
    }
    const bool boundary_only = (rings / ring_step) * grid.sector_count > kMaxShellVerticalLines;
    for (uint32_t sector = 0; sector < grid.sector_count; ++sector) {
      const float theta = static_cast<float>(sector) / static_cast<float>(grid.sector_count) * tau;
      const glm::vec3 dir = u * glm::cos(theta) + v * glm::sin(theta);
      if (!boundary_only) {
        for (uint32_t ring = ring_step; ring < rings; ring += ring_step) {
          const glm::vec3 p = dir * (static_cast<float>(ring) * grid.cell_size);
          draw.line(tp(p - n * half_height), tp(p + n * half_height), line_col(ring));
        }
      }
      /// the boundary vertical always closes the shell
      const glm::vec3 p = dir * outer_radius;
      draw.line(tp(p - n * half_height), tp(p + n * half_height), line_col(rings));
    }

    if (grid.show_axes) {
      draw.line(tp(-n * half_height), tp(n * half_height), grid_axis_color(n));
    }
  }

}  // namespace other
