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
    // [NativeFunction("DrawAABB")]
    // internal static unsafe delegate*<float, float, float, float, float, float, float, float, float, float, NativeBool32, void> NativeDrawAABB;
    // [NativeFunction("DrawOBB")]
    // internal static unsafe delegate*<float, float, float, float, float, float, float, float, float, float, float, float, NativeBool32, void> NativeDrawOBB;
    // [NativeFunction("DrawFrustum")]
    // internal static unsafe delegate*<float, float, float, float, float, float, float, float, float, float, float, float, NativeBool32, void> NativeDrawFrustum;
    // [NativeFunction("DrawTransform")]
    // internal static unsafe delegate*<float, float, float, float, float, float, float, float, float, float, float, float, NativeBool32, void> NativeDrawTransform;
    // [NativeFunction("DrawMesh")]
    internal static unsafe delegate*<ulong, float, float, float, float, float, float, float, float, float, float, float, NativeBool32, void> NativeDrawMesh;
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

    // public static void AABB(Vec3 min, Vec3 max, Vec4 color, DrawTarget target = DrawTarget.Scene)
    // {
    //   unsafe { NativeDrawAABB(min.X, min.Y, min.Z, max.X, max.Y, max.Z, color.X, color.Y, color.Z, color.W, target == DrawTarget.Scene); }
    // }

    // public static void OBB(Vec3 center, Vec3 halfExtents, Vec4 color, Mat4 rotation, DrawTarget target = DrawTarget.Scene)
    // {
    //   unsafe { NativeDrawOBB(center.X, center.Y, center.Z, halfExtents.X, halfExtents.Y, halfExtents.Z, color.X, color.Y, color.Z, color.W,
    //                          rotation.M11, rotation.M12, rotation.M13,
    //                          rotation.M21, rotation.M22, rotation.M23,
    //                          rotation.M31, rotation.M32, rotation.M33,
    //                          target == DrawTarget.Scene); }
    // }

    // public static void Frustum(Vec3 origin, Vec3 forward, Vec3 up, float fovY, float aspectRatio, float nearPlane, float farPlane, Vec4 color, DrawTarget target = DrawTarget.Scene)
    // {
    //   unsafe { NativeDrawFrustum(origin.X, origin.Y, origin.Z,
    //                               forward.X, forward.Y, forward.Z,
    //                               up.X, up.Y, up.Z,
    //                               fovY, aspectRatio, nearPlane, farPlane,
    //                               color.X, color.Y, color.Z,
    //                               target == DrawTarget.Scene); }
    // }

    // public static void Transform(Vec3 position, Mat4 rotation, float scale, DrawTarget target = DrawTarget.Scene)
    // {
    //   unsafe { NativeDrawTransform(position.X, position.Y, position.Z,
    //                                 rotation.M11, rotation.M12, rotation.M13,
    //                                 rotation.M21, rotation.M22, rotation.M23,
    //                                 rotation.M31, rotation.M32, rotation.M33,
    //                                 scale,
    //                                 target == DrawTarget.Scene); }
    // }

    // public static void Mesh(ulong objectId, Vec4 color, Mat4 transform, bool wireframe = false, DrawTarget target = DrawTarget.Scene)
    // {
    //   unsafe { NativeDrawMesh(objectId,
    //                           transform.M11, transform.M12, transform.M13, transform.M14,
    //                           transform.M21, transform.M22, transform.M23, transform.M24,
    //                           transform.M31, transform.M32, transform.M33, transform.M34,
    //                           transform.M41, transform.M42, transform.M43, transform.M44,
    //                           color.X, color.Y, color.Z, color.W,
    //                           wireframe,
    //                           target == DrawTarget.Scene); }
    // }

    /// Draws the grid described by the component, oriented by its object's world transform.
    /// Call every frame the grid should be visible; nothing draws grids implicitly.
    public static void Grid(GridComponent grid, DrawTarget target = DrawTarget.Scene)
    {
      unsafe { NativeDrawGrid(grid.ObjectId, target == DrawTarget.Scene); }
    }
  }
}
