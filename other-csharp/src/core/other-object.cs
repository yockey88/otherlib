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
      Networking.ReplicatedSync.Track(behavior);
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
          Networking.ReplicatedSync.Untrack(behaviors[i]);
          behaviors.RemoveAt(i);
        }
      }
    }

    public void RemoveBehavior(OtherBehavior behavior)
    {
      behavior.OnRemoveFromObject(this);
      Networking.ReplicatedSync.Untrack(behavior);
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

    /// play/stop keeps managed instances alive while the native scene rebuilds; the old
    /// native ids die in the restore, so the binding is reset at the end of the disable
    /// pass and rebound once the restored native object exists
    public void ResetNativeHandle()
    {
      internal_handles.object_id = 0;
      internal_handles.native_handle = IntPtr.Zero;
    }

    public void RebindNativeHandle(IntPtr native_handle)
    {
      internal_handles.native_handle = native_handle;
      internal_handles.object_id = 0;
      internal_handles.object_id = ObjectID;
      OnNativeRebind();
    }

    protected virtual void OnNativeRebind() {}

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

    /// physics dispatch entry points, invoked from native by name with flat primitives
    ///  (no new marshal-table surface); the CollisionInfo is assembled managed-side
    private void CollisionEnter(ulong other_id, float px, float py, float pz, float nx, float ny, float nz)
    {
      CollisionInfo info = new CollisionInfo(other_id, new Vec3(px, py, pz), new Vec3(nx, ny, nz));
      for (int i = 0; i < behaviors.Count; i++)
      {
        behaviors[i].ObjectCollisionEnter(info);
      }
    }
    private void CollisionExit(ulong other_id, float px, float py, float pz, float nx, float ny, float nz)
    {
      CollisionInfo info = new CollisionInfo(other_id, new Vec3(px, py, pz), new Vec3(nx, ny, nz));
      for (int i = 0; i < behaviors.Count; i++)
      {
        behaviors[i].ObjectCollisionExit(info);
      }
    }
    private void TriggerEnter(ulong other_id, float px, float py, float pz, float nx, float ny, float nz)
    {
      CollisionInfo info = new CollisionInfo(other_id, new Vec3(px, py, pz), new Vec3(nx, ny, nz));
      for (int i = 0; i < behaviors.Count; i++)
      {
        behaviors[i].ObjectTriggerEnter(info);
      }
    }
    private void TriggerExit(ulong other_id, float px, float py, float pz, float nx, float ny, float nz)
    {
      CollisionInfo info = new CollisionInfo(other_id, new Vec3(px, py, pz), new Vec3(nx, ny, nz));
      for (int i = 0; i < behaviors.Count; i++)
      {
        behaviors[i].ObjectTriggerExit(info);
      }
    }
    private void JointBreak(float force)
    {
      for (int i = 0; i < behaviors.Count; i++)
      {
        behaviors[i].ObjectJointBreak(force);
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