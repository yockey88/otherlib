using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace OtherCsBindings
{
  [InteropBinding("NativeObjectManager")]
  public class NativeObjectManager
  {
    private static Dictionary<Int64, NativeObject> native_objects = new Dictionary<Int64, NativeObject>();

    [UnmanagedCallersOnly]
    static void AttachNativeObject(Int64 id, IntPtr native_ptr, NativeString class_name)
    {
      try {
        if (native_objects.ContainsKey(id))
        {
          throw new InvalidOperationException($"Native object with ID {id} is already attached.");
        }

        Logger.LogDebug($"Attaching native object with ID {id} and class name {class_name}.");
        native_objects.Add(id, new NativeObject(id, native_ptr, class_name));
      } catch (Exception e) {
        Host.HandleException(e);
      }
    }

    [UnmanagedCallersOnly]
    static void DetachNativeObject(Int64 id, IntPtr native_ptr)
    {
      try {
        if (!native_objects.ContainsKey(id))
        {
          throw new InvalidOperationException($"Native object with ID {id} is not attached.");
        }

        Logger.LogDebug($"Detaching native object with ID {id}.");
        native_objects.Remove(id);
      } catch (Exception e) {
        Host.HandleException(e);
      }
    }
  }
}