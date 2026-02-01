using System;
using System.Collections.Generic;
using OtherCsBindings;

namespace Other.Core
{
  public class OtherObject
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
    public OtherObject(IntPtr nativeHandle)
    {
      behaviors = new List<Behavior>();
      internal_handles = new InternalHandles
      {
        object_id = 0,
        native_handle = nativeHandle
      };
      internal_handles.object_id = ObjectID;
    }

    void AddBehavior(Behavior behavior)
    {
      behavior.OnAddToObject(this);
      behaviors.Add(behavior);
    }

    void RemoveBehavior(string class_name)
    {
      /// todo
    }

    void RemoveBehavior(Behavior behavior)
    {
      behavior.OnRemoveFromObject(this);
      behaviors.Remove(behavior);
    }
  }
}