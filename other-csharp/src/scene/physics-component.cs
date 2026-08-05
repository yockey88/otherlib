using System;
using OtherCsBindings;

namespace Other
{
#nullable enable
  public enum PhysicsBodyType : uint
  {
    Static = 0,
    Kinematic = 1,
    Dynamic = 2,
  }

  [NativeComponent("physics_component")]
  public class PhysicsComponent : Component
  {
    [NativeFunction("GetPhysicsBodyType")]
    internal static unsafe delegate*<UInt64, UInt32> NativeGetPhysicsBodyType;
    [NativeFunction("SetPhysicsBodyType")]
    internal static unsafe delegate*<UInt64, UInt32, void> NativeSetPhysicsBodyType;
    [NativeFunction("GetPhysicsMass")]
    internal static unsafe delegate*<UInt64, float> NativeGetPhysicsMass;
    [NativeFunction("SetPhysicsMass")]
    internal static unsafe delegate*<UInt64, float, void> NativeSetPhysicsMass;
    [NativeFunction("GetPhysicsIsTrigger")]
    internal static unsafe delegate*<UInt64, UInt32> NativeGetPhysicsIsTrigger;
    [NativeFunction("SetPhysicsIsTrigger")]
    internal static unsafe delegate*<UInt64, UInt32, void> NativeSetPhysicsIsTrigger;

    public PhysicsComponent(ulong object_id)
      : base(object_id)
    {
    }

    /// edits write the authored settings; the scene reconciles the live body on the next
    /// frame's revalidation pass
    public PhysicsBodyType BodyType {
      get
      {
        unsafe
        {
          return (PhysicsBodyType)NativeGetPhysicsBodyType(ObjectId);
        }
      }
      set
      {
        unsafe
        {
          NativeSetPhysicsBodyType(ObjectId, (UInt32)value);
        }
      }
    }

    public float Mass {
      get
      {
        unsafe
        {
          return NativeGetPhysicsMass(ObjectId);
        }
      }
      set
      {
        unsafe
        {
          NativeSetPhysicsMass(ObjectId, value);
        }
      }
    }

    public bool IsTrigger {
      get
      {
        unsafe
        {
          return NativeGetPhysicsIsTrigger(ObjectId) != 0;
        }
      }
      set
      {
        unsafe
        {
          NativeSetPhysicsIsTrigger(ObjectId, value ? 1u : 0u);
        }
      }
    }

    /// velocity / force / raycast verbs land with the contacts + queries work
  }
#nullable disable
}
