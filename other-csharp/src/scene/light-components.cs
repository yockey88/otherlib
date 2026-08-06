using System;
using OtherCsBindings;

namespace Other
{
  [NativeComponent("point_light_component")]
  public class PointLightComponent : Component
  {
    [NativeFunction("PointLightGetPosition")]
    internal static unsafe delegate*<UInt64, float*, void> NativePointLightGetPosition;
    [NativeFunction("PointLightSetPosition")]
    internal static unsafe delegate*<UInt64, float, float, float, void> NativePointLightSetPosition;
    [NativeFunction("PointLightGetColor")]
    internal static unsafe delegate*<UInt64, float*, void> NativePointLightGetColor;
    [NativeFunction("PointLightSetColor")]
    internal static unsafe delegate*<UInt64, float, float, float, float, void> NativePointLightSetColor;

    public PointLightComponent(ulong object_id)
      : base(object_id)
    {
    }

    /// local offset from the owning object; the renderer applies the world transform
    public Vec3 Position {
      get
      {
        unsafe
        {
          float* v = stackalloc float[3];
          NativePointLightGetPosition(ObjectId, v);
          return new Vec3(v[0], v[1], v[2]);
        }
      }
      set
      {
        unsafe
        {
          NativePointLightSetPosition(ObjectId, value.X, value.Y, value.Z);
        }
      }
    }

    public Vec4 Color {
      get
      {
        unsafe
        {
          float* v = stackalloc float[4];
          NativePointLightGetColor(ObjectId, v);
          return new Vec4(v[0], v[1], v[2], v[3]);
        }
      }
      set
      {
        unsafe
        {
          NativePointLightSetColor(ObjectId, value.X, value.Y, value.Z, value.W);
        }
      }
    }
  }

  [NativeComponent("direction_light_component")]
  public class DirectionLightComponent : Component
  {
    [NativeFunction("DirectionLightGetDirection")]
    internal static unsafe delegate*<UInt64, float*, void> NativeDirectionLightGetDirection;
    [NativeFunction("DirectionLightSetDirection")]
    internal static unsafe delegate*<UInt64, float, float, float, void> NativeDirectionLightSetDirection;
    [NativeFunction("DirectionLightGetColor")]
    internal static unsafe delegate*<UInt64, float*, void> NativeDirectionLightGetColor;
    [NativeFunction("DirectionLightSetColor")]
    internal static unsafe delegate*<UInt64, float, float, float, float, void> NativeDirectionLightSetColor;

    public DirectionLightComponent(ulong object_id)
      : base(object_id)
    {
    }

    /// TO-the-light convention (matches the sim environment's sun_direction)
    public Vec3 Direction {
      get
      {
        unsafe
        {
          float* v = stackalloc float[3];
          NativeDirectionLightGetDirection(ObjectId, v);
          return new Vec3(v[0], v[1], v[2]);
        }
      }
      set
      {
        unsafe
        {
          NativeDirectionLightSetDirection(ObjectId, value.X, value.Y, value.Z);
        }
      }
    }

    public Vec4 Color {
      get
      {
        unsafe
        {
          float* v = stackalloc float[4];
          NativeDirectionLightGetColor(ObjectId, v);
          return new Vec4(v[0], v[1], v[2], v[3]);
        }
      }
      set
      {
        unsafe
        {
          NativeDirectionLightSetColor(ObjectId, value.X, value.Y, value.Z, value.W);
        }
      }
    }
  }
}
