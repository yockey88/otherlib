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
    [NativeFunction("GetPhysicsLinearVelocity")]
    internal static unsafe delegate*<UInt64, float*, void> NativeGetPhysicsLinearVelocity;
    [NativeFunction("SetPhysicsLinearVelocity")]
    internal static unsafe delegate*<UInt64, float, float, float, void> NativeSetPhysicsLinearVelocity;
    [NativeFunction("GetPhysicsAngularVelocity")]
    internal static unsafe delegate*<UInt64, float*, void> NativeGetPhysicsAngularVelocity;
    [NativeFunction("SetPhysicsAngularVelocity")]
    internal static unsafe delegate*<UInt64, float, float, float, void> NativeSetPhysicsAngularVelocity;
    [NativeFunction("PhysicsAddForce")]
    internal static unsafe delegate*<UInt64, float, float, float, void> NativePhysicsAddForce;
    [NativeFunction("PhysicsAddImpulse")]
    internal static unsafe delegate*<UInt64, float, float, float, void> NativePhysicsAddImpulse;
    [NativeFunction("PhysicsAddTorque")]
    internal static unsafe delegate*<UInt64, float, float, float, void> NativePhysicsAddTorque;

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

    /// runtime verbs against the live body; forces accumulate over the next fixed step,
    /// impulses change velocity immediately. non-dynamic bodies warn and ignore
    public Vec3 LinearVelocity {
      get
      {
        unsafe
        {
          float* v = stackalloc float[3];
          NativeGetPhysicsLinearVelocity(ObjectId, v);
          return new Vec3(v[0], v[1], v[2]);
        }
      }
      set
      {
        unsafe
        {
          NativeSetPhysicsLinearVelocity(ObjectId, value.X, value.Y, value.Z);
        }
      }
    }

    public Vec3 AngularVelocity {
      get
      {
        unsafe
        {
          float* v = stackalloc float[3];
          NativeGetPhysicsAngularVelocity(ObjectId, v);
          return new Vec3(v[0], v[1], v[2]);
        }
      }
      set
      {
        unsafe
        {
          NativeSetPhysicsAngularVelocity(ObjectId, value.X, value.Y, value.Z);
        }
      }
    }

    public void AddForce(Vec3 force)
    {
      unsafe
      {
        NativePhysicsAddForce(ObjectId, force.X, force.Y, force.Z);
      }
    }

    public void AddImpulse(Vec3 impulse)
    {
      unsafe
      {
        NativePhysicsAddImpulse(ObjectId, impulse.X, impulse.Y, impulse.Z);
      }
    }

    public void AddTorque(Vec3 torque)
    {
      unsafe
      {
        NativePhysicsAddTorque(ObjectId, torque.X, torque.Y, torque.Z);
      }
    }
  }
#nullable disable
}
