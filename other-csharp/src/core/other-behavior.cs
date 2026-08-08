using System;
using Other.Core;

namespace Other.Core
{
  /// contact data for collision/trigger callbacks; Point and Normal are world space and the
  /// normal points from this object toward the other. exits carry no point/normal
  public struct CollisionInfo
  {
    public UInt64 OtherObjectId;
    public Vec3 Point;
    public Vec3 Normal;

    public CollisionInfo(UInt64 other_id, Vec3 point, Vec3 normal)
    {
      OtherObjectId = other_id;
      Point = point;
      Normal = normal;
    }
  }

  public abstract class OtherBehavior
  {
    
  #nullable enable
    OtherObject? parent_object = null;
  #nullable disable

    public UInt64 ObjectID
    {
      get
      {
        if (parent_object == null)
        {
          return 0;
        }
        return parent_object!.ObjectID;
      }
    }

    private bool enabled = false;
    public bool Enabled
    {
      get => enabled;
      set
      {
        if (enabled == value)
        {
          return;
        }

        enabled = value;
        if (enabled)
        {
          Enable();
        }
        else
        {
          Disable();
        }
      }
    }

    public OtherObject ParentObject => parent_object;

    public void OnAddToObject(OtherObject obj) 
    {
      parent_object = obj;
      ObjectAwake();
    }
    public void OnRemoveFromObject(OtherObject obj) 
    {
      ObjectRemove();
      parent_object = null;
    }

    public void ObjectAwake()
    {
      Awake();
    }
    public void ObjectRemove()
    {
      Remove();
    }

    public void ObjectUpdate()
    {
      Update();
    }

    public void ObjectLateUpdate()
    {
      LateUpdate();
    }

    public void ObjectFixedUpdate()
    {
      FixedUpdate();
    }

    public void ObjectCollisionEnter(CollisionInfo info)
    {
      CollisionEnter(info);
    }
    public void ObjectCollisionExit(CollisionInfo info)
    {
      CollisionExit(info);
    }
    public void ObjectTriggerEnter(CollisionInfo info)
    {
      TriggerEnter(info);
    }
    public void ObjectTriggerExit(CollisionInfo info)
    {
      TriggerExit(info);
    }
    public void ObjectJointBreak(float force)
    {
      JointBreak(force);
    }

    public void ObjectRender()
    {
      Render();
    }

    /// on load and unload
    protected abstract void Awake();
    protected abstract void Remove();
    /// on enable and disable
    protected abstract void Enable();
    protected abstract void Disable();
    /// primary run loop updating, rendering, physics, etc. goes here
    protected abstract void Update();
    protected abstract void LateUpdate();
    protected abstract void FixedUpdate();

    /// physics callbacks are virtual no-ops: behaviors opt in by overriding. an exit is not
    /// guaranteed if either body was destroyed the same tick
    protected virtual void CollisionEnter(CollisionInfo info) {}
    protected virtual void CollisionExit(CollisionInfo info) {}
    protected virtual void TriggerEnter(CollisionInfo info) {}
    protected virtual void TriggerExit(CollisionInfo info) {}
    protected virtual void JointBreak(float force) {}
    /// per-frame draw hook, called whether or not the scene is playing
    protected virtual void Render() {}
  }
}