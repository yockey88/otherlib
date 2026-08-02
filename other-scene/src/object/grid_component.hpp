/**
 * \file object/grid_component.hpp
 **/
#ifndef OTHER_SCENE_OBJECT_GRID_COMPONENT_HPP
#define OTHER_SCENE_OBJECT_GRID_COMPONENT_HPP

#include <glm/glm.hpp>

#include "core/defines.hpp"
#include "serialization/reflection.hpp"

#include "renderer/debug_draw.hpp"

namespace other {

  /// stored in grid_component::coordinate_system (uint32_t keeps the field scriptable through the ABI)
  enum grid_coordinate_system : uint32_t {
    GRID_COORDINATES_CARTESIAN = 0,
    GRID_COORDINATES_POLAR = 1,
    GRID_COORDINATES_SPHERICAL = 2,
    /// polar layers stacked along the plane normal into a thick disc
    GRID_COORDINATES_CYLINDRICAL = 3,

    NUM_GRID_COORDINATE_SYSTEMS,
  };

  /// stored in grid_component::plane, orients the grid basis relative to the owning object
  enum grid_plane : uint32_t {
    GRID_PLANE_XZ = 0,
    GRID_PLANE_XY = 1,
    GRID_PLANE_YZ = 2,

    NUM_GRID_PLANES,
  };

  /// pure data describing a coordinate grid; nothing draws it implicitly, scripts and drivers
  ///   submit it per frame (e.g. C# Draw.Grid) to the in-scene grid pass or the debug overlay
  struct grid_component {
    bool visible = true;
    bool show_axes = true;

    uint32_t coordinate_system = GRID_COORDINATES_CARTESIAN;
    uint32_t plane = GRID_PLANE_XZ;

    glm::vec3 origin{ 0.f };
    float cell_size = 1.f;

    /// cartesian: half extent in cells, polar/spherical/cylindrical: ring count from the origin
    uint32_t extent = 50;
    uint32_t major_line_every = 10;
    /// polar/cylindrical: radial spokes, spherical: meridians
    uint32_t sector_count = 12;

    /// cylindrical: ring layers stacked above and below the base plane (0 = a flat polar disc)
    uint32_t layer_extent = 4;
    /// cylindrical: distance between stacked layers along the plane normal
    float layer_spacing = 1.f;

    /// in-scene target only: line thickness in pixels
    float line_width = 1.5f;

    glm::vec4 line_color{ 0.28f, 0.30f, 0.34f, 1.f };
    glm::vec4 major_line_color{ 0.48f, 0.52f, 0.60f, 1.f };
  };

  /// emits the grid described by @p grid into the given line stream, oriented by @p world
  void emit_grid_lines(debug_draw& draw, const grid_component& grid, const glm::mat4& world);

  /// emits the lines a cylindrical grid needs beyond its stacked plane layers (verticals at the
  ///   ring/sector intersections and the normal axis); no-op for other coordinate systems
  void emit_grid_shell_lines(debug_draw& draw, const grid_component& grid, const glm::mat4& world);

  /// stacked plane layers the in-scene grid pass draws for @p grid (cylindrical: 2 * layers + 1, else 1)
  uint32_t grid_draw_layer_count(const grid_component& grid);

  /// packs plane layer @p layer (see grid_draw_layer_count) of @p grid into a procedural grid
  ///   submission for renderer::submit_grid, oriented by @p world; cylindrical layers off the base
  ///   plane are offset along the plane normal and draw the full polar pattern without the axes
  grid_draw_data make_grid_draw_data(const grid_component& grid, const glm::mat4& world, uint32_t layer = 0);

  /// world axis identity color for a plane basis axis (X = red, Y = green, Z = blue)
  glm::vec4 grid_axis_color(const glm::vec3& local_axis);

  struct grid_basis {
    glm::vec3 u;
    glm::vec3 v;
    glm::vec3 n;
  };
  grid_basis grid_basis_for_plane(uint32_t plane);

}  // namespace other

OTHER_REFLECT(
  other::grid_component,
  field(visible, other::attr::serializable("Visible")),
  field(show_axes, other::attr::serializable("Show Axes")),
  field(coordinate_system, other::attr::serializable("Coordinate System"),
        other::attr::clamp<uint32_t>(0, other::NUM_GRID_COORDINATE_SYSTEMS - 1)),
  field(plane, other::attr::serializable("Plane"),
        other::attr::clamp<uint32_t>(0, other::NUM_GRID_PLANES - 1)),
  field(origin, other::attr::serializable("Origin")),
  field(cell_size, other::attr::serializable("Cell Size")),
  field(extent, other::attr::serializable("Extent")),
  field(major_line_every, other::attr::serializable("Major Line Every")),
  field(sector_count, other::attr::serializable("Sector Count")),
  field(layer_extent, other::attr::serializable("Layer Extent")),
  field(layer_spacing, other::attr::serializable("Layer Spacing")),
  field(line_width, other::attr::serializable("Line Width"), other::attr::clamp<float>(0.5f, 16.0f)),
  field(line_color, other::attr::serializable("Line Color"), other::attr::clamp<float>(0.0f, 1.0f)),
  field(major_line_color, other::attr::serializable("Major Line Color"), other::attr::clamp<float>(0.0f, 1.0f)))

#endif  // OTHER_SCENE_OBJECT_GRID_COMPONENT_HPP
