using System;
using OtherCsBindings;

namespace Other
{
  /// The scene renders from the camera tagged "main-camera"; the camera struct itself is
  /// the pose source (not the object's transform), so the pose verbs write it directly.
  [NativeComponent("camera_component")]
  public class CameraComponent : Component
  {
    [NativeFunction("CameraGetPosition")]
    internal static unsafe delegate*<UInt64, float*, void> NativeCameraGetPosition;
    [NativeFunction("CameraLookFrom")]
    internal static unsafe delegate*<UInt64, float, float, float, void> NativeCameraLookFrom;
    [NativeFunction("CameraGetDirection")]
    internal static unsafe delegate*<UInt64, float*, void> NativeCameraGetDirection;
    [NativeFunction("CameraLookAt")]
    internal static unsafe delegate*<UInt64, float, float, float, void> NativeCameraLookAt;
    [NativeFunction("CameraLook")]
    internal static unsafe delegate*<UInt64, float, float, float, float, float, float, void> NativeCameraLook;
    [NativeFunction("CameraGetFov")]
    internal static unsafe delegate*<UInt64, float> NativeCameraGetFov;
    [NativeFunction("CameraSetFov")]
    internal static unsafe delegate*<UInt64, float, void> NativeCameraSetFov;
    [NativeFunction("CameraGetClipPlanes")]
    internal static unsafe delegate*<UInt64, float*, float*, void> NativeCameraGetClipPlanes;
    [NativeFunction("CameraSetClipPlanes")]
    internal static unsafe delegate*<UInt64, float, float, void> NativeCameraSetClipPlanes;

    public CameraComponent(ulong object_id)
      : base(object_id)
    {
    }

    /// moving keeps the current view direction
    public Vec3 Position {
      get
      {
        unsafe
        {
          float* v = stackalloc float[3];
          NativeCameraGetPosition(ObjectId, v);
          return new Vec3(v[0], v[1], v[2]);
        }
      }
      set
      {
        unsafe
        {
          NativeCameraLookFrom(ObjectId, value.X, value.Y, value.Z);
        }
      }
    }

    public Vec3 Direction {
      get
      {
        unsafe
        {
          float* v = stackalloc float[3];
          NativeCameraGetDirection(ObjectId, v);
          return new Vec3(v[0], v[1], v[2]);
        }
      }
    }

    /// aims from the current position
    public void LookAt(Vec3 target)
    {
      unsafe
      {
        NativeCameraLookAt(ObjectId, target.X, target.Y, target.Z);
      }
    }

    public void Look(Vec3 position, Vec3 target)
    {
      unsafe
      {
        NativeCameraLook(ObjectId, position.X, position.Y, position.Z, target.X, target.Y, target.Z);
      }
    }

    public float Fov {
      get
      {
        unsafe
        {
          return NativeCameraGetFov(ObjectId);
        }
      }
      set
      {
        unsafe
        {
          NativeCameraSetFov(ObjectId, value);
        }
      }
    }

    public float NearPlane {
      get
      {
        unsafe
        {
          float near = 0f, far = 0f;
          NativeCameraGetClipPlanes(ObjectId, &near, &far);
          return near;
        }
      }
      set
      {
        unsafe
        {
          float near = 0f, far = 0f;
          NativeCameraGetClipPlanes(ObjectId, &near, &far);
          NativeCameraSetClipPlanes(ObjectId, value, far);
        }
      }
    }

    public float FarPlane {
      get
      {
        unsafe
        {
          float near = 0f, far = 0f;
          NativeCameraGetClipPlanes(ObjectId, &near, &far);
          return far;
        }
      }
      set
      {
        unsafe
        {
          float near = 0f, far = 0f;
          NativeCameraGetClipPlanes(ObjectId, &near, &far);
          NativeCameraSetClipPlanes(ObjectId, near, value);
        }
      }
    }
  }
}
