using System;
using OtherCsBindings;

namespace Other
{
  public enum DrawTarget : uint
  {
    /// rendered inside the scene, depth tested against scene geometry
    Scene = 0,
    /// rendered in the debug overlay view on top of the finished frame (editor only)
    Debug = 1,
  }

  public static class Draw
  {
    [NativeFunction("DrawLine")]
    internal static unsafe delegate*<float, float, float, float, float, float, float, float, float, float, NativeBool32, void> NativeDrawLine;
    [NativeFunction("DrawTriangle")]
    internal static unsafe delegate*<float, float, float, float, float, float, float, float, float, float, float, float, float, NativeBool32, void> NativeDrawTriangle;
    [NativeFunction("DrawPoint")]
    internal static unsafe delegate*<float, float, float, float, float, float, float, NativeBool32, void> NativeDrawPoint;
    [NativeFunction("DrawGrid")]
    internal static unsafe delegate*<ulong, NativeBool32, void> NativeDrawGrid;

    public static void Line(Vec3 a, Vec3 b, Vec4 color, DrawTarget target = DrawTarget.Scene)
    {
      unsafe { NativeDrawLine(a.X, a.Y, a.Z, b.X, b.Y, b.Z, color.X, color.Y, color.Z, color.W, target == DrawTarget.Scene); }
    }

    public static void Triangle(Vec3 a, Vec3 b, Vec3 c, Vec4 color, DrawTarget target = DrawTarget.Scene)
    {
      unsafe { NativeDrawTriangle(a.X, a.Y, a.Z, b.X, b.Y, b.Z, c.X, c.Y, c.Z, color.X, color.Y, color.Z, color.W, target == DrawTarget.Scene); }
    }

    public static void Point(Vec3 p, Vec4 color, DrawTarget target = DrawTarget.Scene)
    {
      unsafe { NativeDrawPoint(p.X, p.Y, p.Z, color.X, color.Y, color.Z, color.W, target == DrawTarget.Scene); }
    }

    /// Draws the grid described by the component, oriented by its object's world transform.
    /// Call every frame the grid should be visible; nothing draws grids implicitly.
    public static void Grid(GridComponent grid, DrawTarget target = DrawTarget.Scene)
    {
      unsafe { NativeDrawGrid(grid.ObjectId, target == DrawTarget.Scene); }
    }

    public static void Ray(Vec3 origin, Vec3 direction, float length, Vec4 color, DrawTarget target = DrawTarget.Scene)
    {
      float mag = MathF.Sqrt(direction.X * direction.X + direction.Y * direction.Y + direction.Z * direction.Z);
      if (mag <= 0.00001f || length <= 0.0f)
      {
        return;
      }
      float scale = length / mag;
      Vec3 end = new Vec3(origin.X + direction.X * scale, origin.Y + direction.Y * scale, origin.Z + direction.Z * scale);
      Line(origin, end, color, target);
    }
  }
}
