using System;
using OtherCsBindings;

namespace Other
{
  public static class TransformAccess
  {
    [NativeFunction("TransformGetPosition")]
    internal static unsafe delegate*<UInt64, float*, float*, float*, void> NativeGetPosition;
    [NativeFunction("TransformSetPosition")]
    internal static unsafe delegate*<UInt64, float, float, float, void> NativeSetPosition;
    [NativeFunction("TransformGetRotation")]
    internal static unsafe delegate*<UInt64, float*, float*, float*, float*, void> NativeGetRotation;
    [NativeFunction("TransformSetRotation")]
    internal static unsafe delegate*<UInt64, float, float, float, float, void> NativeSetRotation;
    [NativeFunction("TransformGetScale")]
    internal static unsafe delegate*<UInt64, float*, float*, float*, void> NativeGetScale;
    [NativeFunction("TransformSetScale")]
    internal static unsafe delegate*<UInt64, float, float, float, void> NativeSetScale;
    [NativeFunction("TransformGetWorldMatrix")]
    internal static unsafe delegate*<UInt64, float*, void> NativeGetWorldMatrix;

    public static Vec3 GetPosition(ulong object_id)
    {
      float x = 0, y = 0, z = 0;
      unsafe { NativeGetPosition(object_id, &x, &y, &z); }
      return new Vec3(x, y, z);
    }

    public static void SetPosition(ulong object_id, Vec3 position)
    {
      unsafe { NativeSetPosition(object_id, position.X, position.Y, position.Z); }
    }

    public static Quaternion GetRotation(ulong object_id)
    {
      float x = 0, y = 0, z = 0, w = 0;
      unsafe { NativeGetRotation(object_id, &x, &y, &z, &w); }
      return new Quaternion(x, y, z, w);
    }

    public static void SetRotation(ulong object_id, Quaternion rotation)
    {
      unsafe { NativeSetRotation(object_id, rotation.X, rotation.Y, rotation.Z, rotation.W); }
    }

    public static Vec3 GetScale(ulong object_id)
    {
      float x = 0, y = 0, z = 0;
      unsafe { NativeGetScale(object_id, &x, &y, &z); }
      return new Vec3(x, y, z);
    }

    public static void SetScale(ulong object_id, Vec3 scale)
    {
      unsafe { NativeSetScale(object_id, scale.X, scale.Y, scale.Z); }
    }

    public static Mat4 GetWorldMatrix(ulong object_id)
    {
      float[] m = new float[16];
      unsafe { fixed (float* ptr = m) NativeGetWorldMatrix(object_id, ptr); }
      return new Mat4(
        m[0], m[1], m[2], m[3],
        m[4], m[5], m[6], m[7],
        m[8], m[9], m[10], m[11],
        m[12], m[13], m[14], m[15]
      );
    }
  }
}