using System;

namespace Other.Core
{
  public abstract class Behavior
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

    public abstract void ObjectAwake();
    public abstract void ObjectRemove();

    public abstract void SceneStart();
    public abstract void SceneStop();

    // public abstract void OnStartTransferFields();
    // public abstract void OnEndTransferFields();

    public abstract void ObjectUpdate();
    public abstract void ObjectLateUpdate();
    public abstract void ObjectFixedUpdate();
  }

  public abstract class OtherScriptedBehavior : Behavior
  {


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

    public override void ObjectAwake()
    {
      OnAwake();
    }
    public override void ObjectRemove()
    {
      OnRemove();
    }

    public virtual void OnAwake() { }
    public virtual void OnRemove() { }

    public override void SceneStart()
    {
      OnStart();
    }
    public override void SceneStop()
    { 
      OnStop();
    }

    public virtual void OnStart() { }
    public virtual void OnStop() { }

    // public virtual void OnStartTransferFields() { }
    // public virtual void OnEndTransferFields() { }

    public override void ObjectUpdate()
    {
      OnUpdate();
    }
    public override void ObjectLateUpdate()
    {
      OnLateUpdate();
    }
    public override void ObjectFixedUpdate()
    {
      OnFixedUpdate();
    }
    public virtual void OnUpdate() { }
    public virtual void OnLateUpdate() { }
    public virtual void OnFixedUpdate() { }

  }
}