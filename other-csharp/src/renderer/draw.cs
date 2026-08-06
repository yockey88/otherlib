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
    [NativeFunction("DrawRay")]
    internal static unsafe delegate*<float, float, float, float, float, float, float, float, float, float, NativeBool32, void> NativeDrawRay;
    [NativeFunction("DrawArrow")]
    internal static unsafe delegate*<float, float, float, float, float, float, float, float, float, float, NativeBool32, void> NativeDrawArrow;
    [NativeFunction("DrawSphere")]
    internal static unsafe delegate*<float, float, float, float, float, float, float, float, NativeBool32, void> NativeDrawSphere;
    [NativeFunction("DrawAABB")]
    internal static unsafe delegate*<float, float, float, float, float, float, float, float, float, float, NativeBool32, void> NativeDrawAABB;
    [NativeFunction("DrawOBB")]
    internal static unsafe delegate*<float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, NativeBool32, void> NativeDrawOBB;
    [NativeFunction("DrawFrustum")]
    internal static unsafe delegate*<float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, NativeBool32, void> NativeDrawFrustum;
    [NativeFunction("DrawTransform")]
    internal static unsafe delegate*<float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, NativeBool32, void> NativeDrawTransform;
    [NativeFunction("DrawMesh")]
    internal static unsafe delegate*<ulong, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, NativeBool32, NativeBool32, void> NativeDrawMesh;
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

    public static void Ray(Vec3 origin, Vec3 direction, float length, Vec4 color, DrawTarget target = DrawTarget.Scene)
    {
      float mag = MathF.Sqrt(direction.X * direction.X + direction.Y * direction.Y + direction.Z * direction.Z);
      if (mag <= 0.00001f || length <= 0.0f)
      {
        return;
      }
      float scale = length / mag;
      Vec3 end = new Vec3(origin.X + direction.X * scale, origin.Y + direction.Y * scale, origin.Z + direction.Z * scale);
      unsafe { NativeDrawRay(origin.X, origin.Y, origin.Z, end.X, end.Y, end.Z, color.X, color.Y, color.Z, color.W, target == DrawTarget.Scene); }
    }

    public static void Arrow(Vec3 origin, Vec3 direction, float length, Vec4 color, DrawTarget target = DrawTarget.Scene)
    {
      float mag = MathF.Sqrt(direction.X * direction.X + direction.Y * direction.Y + direction.Z * direction.Z);
      if (mag <= 0.00001f || length <= 0.0f)
      {
        return;
      }
      float scale = length / mag;
      Vec3 end = new Vec3(origin.X + direction.X * scale, origin.Y + direction.Y * scale, origin.Z + direction.Z * scale);
      unsafe { NativeDrawArrow(origin.X, origin.Y, origin.Z, end.X, end.Y, end.Z, color.X, color.Y, color.Z, color.W, target == DrawTarget.Scene); }
    }

    public static void Sphere(Vec3 p, float radius, Vec4 color, DrawTarget target = DrawTarget.Scene) 
    {
      unsafe { NativeDrawSphere(p.X, p.Y, p.Z, radius, color.X, color.Y, color.Z, color.W, target == DrawTarget.Scene); }
    }

    public static void AABB(Vec3 min, Vec3 max, Vec4 color, DrawTarget target = DrawTarget.Scene)
    {
      unsafe { NativeDrawAABB(min.X, min.Y, min.Z, max.X, max.Y, max.Z, color.X, color.Y, color.Z, color.W, target == DrawTarget.Scene); }
    }

    /// draws a unit cube (±0.5) carried by the transform — bake center/extents/rotation into it
    public static void OBB(Mat4 transform, Vec4 color, DrawTarget target = DrawTarget.Scene)
    {
      unsafe { NativeDrawOBB(transform.M11, transform.M12, transform.M13, transform.M14,
                             transform.M21, transform.M22, transform.M23, transform.M24,
                             transform.M31, transform.M32, transform.M33, transform.M34,
                             transform.M41, transform.M42, transform.M43, transform.M44,
                             color.X, color.Y, color.Z, color.W,
                             target == DrawTarget.Scene); }
    }

    /// expects the INVERSE view-projection matrix of the frustum to outline
    public static void Frustum(Mat4 inverseViewProjection, Vec4 color, DrawTarget target = DrawTarget.Scene)
    {
      unsafe { NativeDrawFrustum(inverseViewProjection.M11, inverseViewProjection.M12, inverseViewProjection.M13, inverseViewProjection.M14,
                                 inverseViewProjection.M21, inverseViewProjection.M22, inverseViewProjection.M23, inverseViewProjection.M24,
                                 inverseViewProjection.M31, inverseViewProjection.M32, inverseViewProjection.M33, inverseViewProjection.M34,
                                 inverseViewProjection.M41, inverseViewProjection.M42, inverseViewProjection.M43, inverseViewProjection.M44,
                                 color.X, color.Y, color.Z, color.W,
                                 target == DrawTarget.Scene); }
    }

    /// R/G/B axis gizmo at the transform's origin
    public static void Transform(Mat4 transform, float scale = 1.0f, DrawTarget target = DrawTarget.Scene)
    {
      unsafe { NativeDrawTransform(transform.M11, transform.M12, transform.M13, transform.M14,
                                   transform.M21, transform.M22, transform.M23, transform.M24,
                                   transform.M31, transform.M32, transform.M33, transform.M34,
                                   transform.M41, transform.M42, transform.M43, transform.M44,
                                   scale,
                                   target == DrawTarget.Scene); }
    }

    /// draws the render mesh of the object at an arbitrary transform (debug ghosts,
    /// placement previews); objects without an uploaded model draw nothing
    public static void Mesh(ulong objectId, Mat4 transform, Vec4 color, bool wireframe = false, DrawTarget target = DrawTarget.Scene)
    {
      unsafe { NativeDrawMesh(objectId,
                              transform.M11, transform.M12, transform.M13, transform.M14,
                              transform.M21, transform.M22, transform.M23, transform.M24,
                              transform.M31, transform.M32, transform.M33, transform.M34,
                              transform.M41, transform.M42, transform.M43, transform.M44,
                              color.X, color.Y, color.Z, color.W,
                              wireframe,
                              target == DrawTarget.Scene); }
    }

    /// Draws the grid described by the component, oriented by its object's world transform.
    /// Call every frame the grid should be visible; nothing draws grids implicitly.
    public static void Grid(GridComponent grid, DrawTarget target = DrawTarget.Scene)
    {
      unsafe { NativeDrawGrid(grid.ObjectId, target == DrawTarget.Scene); }
    }
  }
}
