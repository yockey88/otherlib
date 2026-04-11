using System;
using OtherCsBindings;

namespace Other
{
  public class SceneObject : Core.OtherObject
  {
    [NativeFunction("GetComponent")]
    internal static unsafe delegate*<nint, UInt64, nint, void> NativeGetComponent;

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

#nullable enable
    public T? GetComponent<T>() where T : Core.OtherBehavior
    {
      return Scene.GetComponent<T>(ObjectID);
    }
#nullable disable

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