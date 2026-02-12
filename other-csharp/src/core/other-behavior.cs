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

    private bool enabled = true;
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
    protected abstract void Awake();

    public void ObjectRemove()
    {
      Remove();
    }
    protected abstract void Remove();

    protected abstract void Enable();
    protected abstract void Disable();

    public void ObjectUpdate()
    {
      Update();
    }
    protected abstract void Update();

    public void ObjectLateUpdate()
    {
      LateUpdate();
    }
    protected abstract void LateUpdate();

    public void ObjectFixedUpdate()
    {
      FixedUpdate();
    }
    protected abstract void FixedUpdate();
  }
}