using System;
using OtherCsBindings;

namespace Other
{
#nullable enable
  public struct RaycastHit
  {
    public UInt64 ObjectId;
    public Vec3 Point;
    public Vec3 Normal;
    public float Distance;
  }

  public static class Physics
  {
    [NativeFunction("PhysicsRaycast")]
    internal static unsafe delegate*<float, float, float, float, float, float, float, UInt64*, float*, float*, float*, UInt32> NativePhysicsRaycast;

    /// closest hit against the active scene's physics world; sensors are triggers, not
    /// surfaces, and never hit. false = nothing within max_distance
    public static bool Raycast(Vec3 origin, Vec3 direction, float max_distance, out RaycastHit hit)
    {
      unsafe
      {
        UInt64 hit_object = 0;
        float distance = 0f;
        float* point = stackalloc float[3];
        float* normal = stackalloc float[3];

        UInt32 result = NativePhysicsRaycast(origin.X, origin.Y, origin.Z,
                                             direction.X, direction.Y, direction.Z,
                                             max_distance, &hit_object, point, normal, &distance);
        hit = new RaycastHit {
          ObjectId = hit_object,
          Point = new Vec3(point[0], point[1], point[2]),
          Normal = new Vec3(normal[0], normal[1], normal[2]),
          Distance = distance,
        };
        return result != 0;
      }
    }
  }
#nullable disable
}
