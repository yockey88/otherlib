using System;
using System.Collections.Generic;
using OtherCsBindings;

namespace Other
{
  public class SceneObject : Core.OtherObject
  {
    private SceneObjectHandle handle;
    public SceneObjectHandle ObjectHandle => handle;

    public SceneObject(IntPtr native_handle)
      : base(native_handle, ObjectType.SceneObject)
    {
      handle = new SceneObjectHandle(ObjectID);
    }

    public T AddBehavior<T>() where T : Core.OtherBehavior, new()
    {
      T behavior = new T();
      AddBehavior(behavior);
      return behavior;
    }

    public override void OnStart()
    {
    }

    public override void OnStop()
    {
      
    }

    public override void OnUpdate()
    {
      
    }

    public override void OnLateUpdate()
    {
      
    }

    public override void OnFixedUpdate()
    {
      
    }
  }
}