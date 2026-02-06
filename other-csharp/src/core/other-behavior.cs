using System;

namespace Other.Core
{
  public interface Behavior
  {
    UInt64 ObjectID { get; }

    public abstract void OnAddToObject(OtherObject obj);
    public abstract void OnRemoveFromObject(OtherObject obj);

    public abstract void OnObjectAwake();
    public abstract void OnObjectDestroy();

    public abstract void OnSceneStart();
    public abstract void OnSceneStop();

    public abstract void OnStartTransferFields();
    public abstract void OnEndTransferFields();

    public abstract void OnObjectUpdate();
    public abstract void OnObjectLateUpdate();
    public abstract void OnObjectFixedUpdate();
  }

  public abstract class OtherScriptedBehavior : Behavior
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

    public OtherScriptedBehavior()
    {
      RegisterScriptType();
    }

    public void RegisterScriptType()
    {
      /// \todo if script type already registered, skip
      RegisterScriptFields();
      RegisterScriptMethods();
    }
    public abstract void RegisterScriptFields();
    public abstract void RegisterScriptMethods();

    public virtual void OnAddToObject(OtherObject obj)
    {
      parent_object = obj;
      OnObjectAwake();
    }
    public virtual void OnRemoveFromObject(OtherObject obj)
    {
      OnObjectDestroy();
      parent_object = null;
    }

    public virtual void OnObjectAwake()
    {
      OnAwake();
    }
    public virtual void OnAwake() { }

    public virtual void OnObjectDestroy()
    {
      OnDestroy();
    }
    public virtual void OnDestroy() { }

    public virtual void OnSceneStart()
    {
      OnStart();
    }
    public virtual void OnStart() { }

    public virtual void OnSceneStop()
    { 
      OnStop();
    }
    public virtual void OnStop() { }

    public virtual void OnStartTransferFields() { }
    public virtual void OnEndTransferFields() { }

    public virtual void OnObjectUpdate()
    {
      OnUpdate();
    }
    public virtual void OnUpdate() { }

    public virtual void OnObjectLateUpdate()
    {
      OnLateUpdate();
    }
    public virtual void OnLateUpdate() { }

    public virtual void OnObjectFixedUpdate()
    {
      OnFixedUpdate();
    }
    public virtual void OnFixedUpdate() { }

  }
}