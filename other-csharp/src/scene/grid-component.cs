using System;

namespace Other
{
  public enum GridCoordinateSystem : uint
  {
    Cartesian = 0,
    Polar = 1,
    Spherical = 2,
    /// polar layers stacked along the plane normal into a thick disc
    Cylindrical = 3,
  }

  public enum GridPlane : uint
  {
    XZ = 0,
    XY = 1,
    YZ = 2,
  }

  [NativeComponent("grid_component")]
  public class GridComponent : Component
  {
    public GridComponent(ulong object_id)
      : base(object_id)
    {
    }

    [NativeField("visible")]
    public bool Visible { get => GetBool(); set => SetBool(value); }
    [NativeField("show_axes")]
    public bool ShowAxes { get => GetBool(); set => SetBool(value); }

    [NativeField("coordinate_system")]
    public GridCoordinateSystem CoordinateSystem { get => (GridCoordinateSystem)GetU32(); set => SetU32((uint)value); }
    [NativeField("plane")]
    public GridPlane Plane { get => (GridPlane)GetU32(); set => SetU32((uint)value); }

    [NativeField("origin")]
    public Vec3 Origin { get => GetVec3(); set => SetVec3(value); }
    [NativeField("cell_size")]
    public float CellSize { get => GetF32(); set => SetF32(value); }

    [NativeField("extent")]
    public uint Extent { get => GetU32(); set => SetU32(value); }
    [NativeField("major_line_every")]
    public uint MajorLineEvery { get => GetU32(); set => SetU32(value); }
    [NativeField("sector_count")]
    public uint SectorCount { get => GetU32(); set => SetU32(value); }

    /// Cylindrical only: ring layers stacked above and below the base plane (0 = a flat polar disc).
    [NativeField("layer_extent")]
    public uint LayerExtent { get => GetU32(); set => SetU32(value); }
    /// Cylindrical only: distance between stacked layers along the plane normal.
    [NativeField("layer_spacing")]
    public float LayerSpacing { get => GetF32(); set => SetF32(value); }

    [NativeField("line_width")]
    public float LineWidth { get => GetF32(); set => SetF32(value); }

    [NativeField("line_color")]
    public Vec4 LineColor { get => GetVec4(); set => SetVec4(value); }
    [NativeField("major_line_color")]
    public Vec4 MajorLineColor { get => GetVec4(); set => SetVec4(value); }
  }
}
