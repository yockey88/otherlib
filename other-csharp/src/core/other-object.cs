using System;
using System.Collections.Generic;
using OtherCsBindings;

namespace Other.Core
{
  public abstract class OtherObject
  {
    private struct InternalHandles
    {
      public UInt64 object_id;
      public IntPtr native_handle;
    }
    private InternalHandles internal_handles;

    [NativeFunction("GetObjectID")]
    internal static unsafe delegate*<nint, UInt64*, void> NativeGetObjectID;

    public UInt64 ObjectID
    {
      get {
        if (internal_handles.native_handle == IntPtr.Zero)
        {
          return 0;
        }
        if (internal_handles.object_id != 0)
        {
          return internal_handles.object_id;
        }
        unsafe { 
          if (NativeGetObjectID == null)
          {
            throw new InvalidOperationException("Native function GetObjectID is not initialized.");
          } 
          UInt64 ret = 0;
          NativeGetObjectID(internal_handles.native_handle, &ret);
          return ret;
        }
      }
    }

    private List<Behavior> behaviors;
    public OtherObject(IntPtr native_handle)
    {
      behaviors = new List<Behavior>();
      internal_handles = new InternalHandles
      {
        object_id = 0,
        native_handle = native_handle
      };
      internal_handles.object_id = ObjectID;
    }

    public void AddBehavior(Behavior behavior)
    {
      behavior.OnAddToObject(this);
      behaviors.Add(behavior);
    }

    public void RemoveBehavior(string class_name)
    {
      behaviors.RemoveAll(b => b.GetType().Name == class_name);
    }

    public void RemoveBehavior(Behavior behavior)
    {
      behavior.OnRemoveFromObject(this);
      behaviors.Remove(behavior);
    }

    public void SceneStart()
    {
      OnStart();
      for (int i = 0; i < behaviors.Count; i++)
      {
        behaviors[i].SceneStart();
      }
    }

    public void SceneStop()
    {
      OnStop();
      for (int i = 0; i < behaviors.Count; i++)
      {
        behaviors[i].SceneStop();
      }
    }

    public void Update()
    {
      OnUpdate();
      for (int i = 0; i < behaviors.Count; i++)
      {
        behaviors[i].ObjectUpdate();
      }
    }
    private void LateUpdate()
    {
      OnLateUpdate();
      for (int i = 0; i < behaviors.Count; i++)
      {
        behaviors[i].ObjectLateUpdate();
      }
    }
    private void FixedUpdate()
    {
      OnFixedUpdate();
      for (int i = 0; i < behaviors.Count; i++)
      {
        behaviors[i].ObjectFixedUpdate();
      }
    }

    public abstract void OnStart();
    public abstract void OnStop();

    public abstract void OnUpdate();
    public abstract void OnLateUpdate();
    public abstract void OnFixedUpdate();
  }
}