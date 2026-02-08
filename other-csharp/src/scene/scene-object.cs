using System;
using System.Collections.Generic;
using OtherCsBindings;

namespace Other
{
  public class SceneObject : Core.OtherObject
  {
    private SceneObjectHandle handle;
    public SceneObject(IntPtr native_handle)
      : base(native_handle)
    {
      handle = new SceneObjectHandle(ObjectID);
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