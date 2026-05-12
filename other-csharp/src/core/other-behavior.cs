using System;
using Other.Core;

namespace Other.Core
{
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
    /// protected virtual void Render() {}
  }
}