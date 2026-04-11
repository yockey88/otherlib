using System;
using OtherCsBindings;

namespace Other
{
  public static class Scene
  {
    private const int kMaxObjectQueryCount = 4096;

    [NativeFunction("SceneCreateObject")]
    internal static unsafe delegate*<NativeString, float, float, float, UInt64> NativeCreateObject;
    [NativeFunction("SceneDestroyObject")]
    internal static unsafe delegate*<UInt64, void> NativeDestroyObject;
    [NativeFunction("SceneHasObject")]
    internal static unsafe delegate*<UInt64, NativeBool32> NativeHasObject;
    [NativeFunction("SceneGetObjectName")]
    internal static unsafe delegate*<UInt64, NativeString> NativeGetObjectName;
    [NativeFunction("SceneSetObjectName")]
    internal static unsafe delegate*<UInt64, NativeString, void> NativeSetObjectName;
    [NativeFunction("SceneGetObjectIds")]
    internal static unsafe delegate*<UInt64*, int*, int, void> NativeGetObjectIds;
    [NativeFunction("SceneGetObjectCount")]
    internal static unsafe delegate*<UInt64> NativeGetObjectCount;
    [NativeFunction("SceneFindObjectByName")]
    internal static unsafe delegate*<NativeString, UInt64> NativeFindObjectByName;
    
    
    [NativeFunction("SceneGetParentId")]
    internal static unsafe delegate*<UInt64, UInt64> NativeGetParentId;
    [NativeFunction("SceneGetChildrenIds")]
    internal static unsafe delegate*<UInt64, UInt64*, int*, int, void> NativeGetChildrenIds;

    [NativeFunction("SceneObjectHasTag")]
    internal static unsafe delegate*<UInt64, NativeString, NativeBool32> NativeObjectHasTag;
    [NativeFunction("SceneAddObjectTag")]
    internal static unsafe delegate*<UInt64, NativeString, void> NativeAddObjectTag;
    [NativeFunction("SceneRemoveObjectTag")]
    internal static unsafe delegate*<UInt64, NativeString, void> NativeRemoveObjectTag;

    [NativeFunction("SceneGetObjectVisible")]
    internal static unsafe delegate*<UInt64, NativeBool32> NativeGetObjectVisible;
    [NativeFunction("SceneSetObjectVisible")]
    internal static unsafe delegate*<UInt64, NativeBool32, void> NativeSetObjectVisible;

    [NativeFunction("SceneGetComponent")]
    internal static unsafe delegate*<nint, UInt64, UInt32, void> NativeGetComponent;

    public static ulong CreateObject(string name, Vec3 position = default)
    {
      unsafe { return NativeCreateObject(name, position.X, position.Y, position.Z); }
    }

    public static void DestroyObject(ulong id)
    {
      unsafe { NativeDestroyObject(id); }
    }

    public static bool HasObject(ulong id)
    {
      unsafe { return NativeHasObject(id); }
    }

    public static string GetObjectName(ulong id)
    {
      unsafe
      {
        NativeString result = NativeGetObjectName(id);
        return result.ToString();
      }
    }

    public static void SetObjectName(ulong id, string name)
    {
      unsafe { NativeSetObjectName(id, name); }
    }

    public static ulong GetObjectCount()
    {
      unsafe { return NativeGetObjectCount(); }
    }

    public static ulong FindObjectByName(string name)
    {
      unsafe { return NativeFindObjectByName(name); }
    }

        public static ulong[] GetAllObjectIds()
    {
      unsafe
      {
        UInt64* buffer = stackalloc UInt64[kMaxObjectQueryCount];
        int count = 0;
        NativeGetObjectIds(buffer, &count, kMaxObjectQueryCount);
        ulong[] result = new ulong[count];
        for (int i = 0; i < count; i++)
          result[i] = buffer[i];
        return result;
      }
    }

    public static ulong GetParentId(ulong id)
    {
      unsafe { return NativeGetParentId(id); }
    }

    public static ulong[] GetChildrenIds(ulong id)
    {
      unsafe
      {
        UInt64* buffer = stackalloc UInt64[kMaxObjectQueryCount];
        int count = 0;
        NativeGetChildrenIds(id, buffer, &count, kMaxObjectQueryCount);
        ulong[] result = new ulong[count];
        for (int i = 0; i < count; i++)
          result[i] = buffer[i];
        return result;
      }
    }

    // -- Tags

    public static bool ObjectHasTag(ulong id, string tag) { unsafe { return NativeObjectHasTag(id, tag); } }
    public static void AddObjectTag(ulong id, string tag) { unsafe { NativeAddObjectTag(id, tag); } }
    public static void RemoveObjectTag(ulong id, string tag) { unsafe { NativeRemoveObjectTag(id, tag); } }

    // -- Visibility

    public static bool GetObjectVisible(ulong id) { unsafe { return NativeGetObjectVisible(id); } }
    public static void SetObjectVisible(ulong id, bool visible) { unsafe { NativeSetObjectVisible(id, visible); } }

#nullable enable
    // -- Components
    public static T? GetComponent<T>(ulong object_id) where T : Core.OtherBehavior
    {
      string component_name = typeof(T).FullName!;

      T? component = null;
      unsafe
      {
        IntPtr result_ptr = IntPtr.Zero;
        // NativeGetComponent((nint)component_name, object_id, (UInt32)MarshalMode.ManagedToNativeIn, &result_ptr);
        if (result_ptr != IntPtr.Zero)
        {
          return OtherMemory.MarshalPointer<T>(result_ptr)!;
        }
      }
      return component;
    }
#nullable disable
  }
}