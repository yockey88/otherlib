using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using OtherCsBindings;

namespace Other.Core
{
  public abstract class OtherObject
  {
    public enum ObjectType
    {
      SceneObject = 0,
      // EditorObject, UIObject, etc...
    }

    private ObjectType object_type;
    public ObjectType Type => object_type;

    private struct InternalHandles
    {
      public UInt64 object_id;
      public IntPtr native_handle;
    }
    private InternalHandles internal_handles;
    public IntPtr NativeHandle => internal_handles.native_handle;


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

    private List<OtherBehavior> behaviors;
    public int BehaviorCount => behaviors.Count;

    public OtherObject(IntPtr native_handle, ObjectType type)
    {
      object_type = type;

      behaviors = new List<OtherBehavior>();
      internal_handles = new InternalHandles
      {
        object_id = 0,
        native_handle = native_handle
      };
      internal_handles.object_id = ObjectID;
    }

    public void AddBehavior(OtherBehavior behavior)
    {
      behavior.OnAddToObject(this);
      behaviors.Add(behavior);
    }

    public void AddNativeBehavior(IntPtr native_handle)
    {
      GCHandle behavior_handle = GCHandle.FromIntPtr(native_handle);
      if (behavior_handle.Target is OtherBehavior behavior)
      {
        AddBehavior(behavior);
      }
      else
      {
        throw new InvalidOperationException("The provided native handle does not point to a valid OtherBehavior instance.");
      }
    }

    public void RemoveBehavior(string class_name)
    {
      for (int i = behaviors.Count - 1; i >= 0; i--)
      {
        if (behaviors[i].GetType().Name == class_name || behaviors[i].GetType().FullName == class_name)
        {
          behaviors[i].OnRemoveFromObject(this);
          behaviors.RemoveAt(i);
        }
      }
    }

    public void RemoveBehavior(OtherBehavior behavior)
    {
      behavior.OnRemoveFromObject(this);
      behaviors.Remove(behavior);
    }

    public void RemoveAllBehaviors()
    {
      for (int i = behaviors.Count - 1; i >= 0; i--)
      {
        RemoveBehavior(behaviors[i]);
      }
      behaviors.Clear();
    }

    public T GetBehavior<T>() where T : OtherBehavior
    {
      for (int i = 0; i < behaviors.Count; i++)
      {
        if (behaviors[i] is T typed)
        {
          return typed;
        }
      }
      return null;
    }

    public bool HasBehavior<T>() where T : OtherBehavior
    {
      for (int i = 0; i < behaviors.Count; i++)
      {
        if (behaviors[i] is T)
        {
          return true;
        }
      }
      return false;
    }

    public bool HasBehavior(string class_name)
    {
      for (int i = 0; i < behaviors.Count; i++)
      {
        if (behaviors[i].GetType().Name == class_name || behaviors[i].GetType().FullName == class_name)
        {
          return true;
        }
      }
      return false;
    }

    public string[] GetBehaviorTypeNames()
    {
      string[] names = new string[behaviors.Count];
      for (int i = 0; i < behaviors.Count; i++)
      {
        names[i] = behaviors[i].GetType().FullName;
      }
      return names;
    }

    public void SceneStart()
    {
      OnStart();
      for (int i = 0; i < behaviors.Count; i++)
      {
        behaviors[i].Enabled = true;
      }
    }

    public void SceneStop()
    {
      OnStop();
      for (int i = 0; i < behaviors.Count; i++)
      {
        behaviors[i].Enabled = false;
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
    /// invoked from native every frame, whether or not the scene is playing
    public void Render()
    {
      OnRender();
      for (int i = 0; i < behaviors.Count; i++)
      {
        behaviors[i].ObjectRender();
      }
    }

    public abstract void OnStart();
    public abstract void OnStop();

    public abstract void OnUpdate();
    public abstract void OnLateUpdate();
    public abstract void OnFixedUpdate();
    public virtual void OnRender() {}

    public virtual void OnDestroy() {}
  }
}