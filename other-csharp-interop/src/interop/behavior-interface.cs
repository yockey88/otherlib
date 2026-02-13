using System;
using System.Collections;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.InteropServices;

#nullable enable
namespace OtherCsBindings
{
  /// <summary>
  /// Generic interop interface for managing behaviors on managed objects.
  /// 
  /// This class has NO knowledge of OtherObject, Behavior, SceneObject, or any 
  /// types in the OtherCs library. All operations are performed via reflection
  /// against the managed object behind the GCHandle.
  /// 
  /// Expected contract on the target object (discovered via reflection):
  ///   - Method: void AddBehavior({some_base_type} behavior)
  ///   - Method: void RemoveBehavior(string class_name) 
  ///   - Method: void RemoveAllBehaviors()
  ///   - Property: int BehaviorCount { get; }
  ///   - Method: bool HasBehavior(string class_name)
  ///   - Method: string[] GetBehaviorTypeNames()
  /// </summary>
  [InteropBinding("BehaviorInterface")]
  internal static class BehaviorInterface
  {
    /// <summary>
    /// Creates an instance of the specified behavior type and adds it to the 
    /// parent managed object by invoking its "AddBehavior" method via reflection.
    /// Returns a GCHandle (IntPtr) to the created behavior instance.
    /// </summary>
    /// <param name="parent_handle">GCHandle to the parent managed object</param>
    /// <param name="behavior_type_name">Fully qualified or short type name of the behavior</param>
    /// <returns>GCHandle to the created behavior, or IntPtr.Zero on failure</returns>
    [UnmanagedCallersOnly]
    internal static unsafe IntPtr AddBehavior(IntPtr parent_handle, NativeString behavior_type_name)
    {
      try
      {
        string? type_name = behavior_type_name;
        if (string.IsNullOrEmpty(type_name))
        {
          Logger.LogError("BehaviorInterface.AddBehavior: behavior type name is null or empty.");
          return IntPtr.Zero;
        }

        object? parent = GCHandle.FromIntPtr(parent_handle).Target;
        if (parent == null)
        {
          Logger.LogError("BehaviorInterface.AddBehavior: parent object handle is null.");
          return IntPtr.Zero;
        }

        // resolve the behavior type across all loaded assemblies
        Type? behavior_type = ResolveType(type_name);
        if (behavior_type == null)
        {
          Logger.LogError($"BehaviorInterface.AddBehavior: failed to resolve behavior type '{type_name}'.");
          return IntPtr.Zero;
        }

        // create an instance of the behavior type
        object? behavior_instance = TypeInterface.CreateInstance(behavior_type);
        if (behavior_instance == null)
        {
          Logger.LogError($"BehaviorInterface.AddBehavior: failed to create instance of type '{type_name}'.");
          return IntPtr.Zero;
        }

        // find and invoke "AddBehavior" on the parent via reflection
        Type parent_type = parent.GetType();
        MethodInfo? add_method = FindAddBehaviorMethod(parent_type, behavior_instance.GetType());
        if (add_method == null)
        {
          Logger.LogError($"BehaviorInterface.AddBehavior: could not find 'AddBehavior' method on type '{parent_type.FullName}' " +
                          $"that accepts parameter of type '{behavior_instance.GetType().FullName}'.");
          return IntPtr.Zero;
        }

        add_method.Invoke(parent, new object[] { behavior_instance });

        // allocate a GCHandle for the behavior and register it for cleanup
        GCHandle behavior_handle = GCHandle.Alloc(behavior_instance, GCHandleType.Normal);
        Assembly behavior_assembly = behavior_type.Assembly;
        AssemblyLoader.RegisterHandle(behavior_assembly, behavior_handle);

        Logger.LogDebug($"BehaviorInterface.AddBehavior: added behavior '{type_name}' to object of type '{parent_type.FullName}'.");
        return GCHandle.ToIntPtr(behavior_handle);
      }
      catch (Exception e)
      {
        Host.HandleException(e);
        return IntPtr.Zero;
      }
    }

    /// <summary>
    /// Removes a behavior from the parent managed object by type name,
    /// invoking "RemoveBehavior(string)" via reflection.
    /// </summary>
    [UnmanagedCallersOnly]
    internal static unsafe void RemoveBehavior(IntPtr parent_handle, NativeString behavior_type_name)
    {
      try
      {
        string? type_name = behavior_type_name;
        if (string.IsNullOrEmpty(type_name))
        {
          Logger.LogError("BehaviorInterface.RemoveBehavior: behavior type name is null or empty.");
          return;
        }

        object? parent = GCHandle.FromIntPtr(parent_handle).Target;
        if (parent == null)
        {
          Logger.LogError("BehaviorInterface.RemoveBehavior: parent object handle is null.");
          return;
        }

        Type parent_type = parent.GetType();
        MethodInfo? remove_method = parent_type.GetMethod("RemoveBehavior",
          BindingFlags.Public | BindingFlags.Instance,
          null, new Type[] { typeof(string) }, null);

        if (remove_method == null)
        {
          Logger.LogError($"BehaviorInterface.RemoveBehavior: could not find 'RemoveBehavior(string)' on type '{parent_type.FullName}'.");
          return;
        }

        remove_method.Invoke(parent, new object[] { type_name });
        Logger.LogDebug($"BehaviorInterface.RemoveBehavior: removed behavior '{type_name}' from object of type '{parent_type.FullName}'.");
      }
      catch (Exception e)
      {
        Host.HandleException(e);
      }
    }

    /// <summary>
    /// Removes all behaviors from the parent managed object by invoking 
    /// "RemoveAllBehaviors()" via reflection.
    /// </summary>
    [UnmanagedCallersOnly]
    internal static unsafe void RemoveAllBehaviors(IntPtr parent_handle)
    {
      try
      {
        object? parent = GCHandle.FromIntPtr(parent_handle).Target;
        if (parent == null)
        {
          Logger.LogError("BehaviorInterface.RemoveAllBehaviors: parent object handle is null.");
          return;
        }

        Type parent_type = parent.GetType();
        MethodInfo? remove_all = parent_type.GetMethod("RemoveAllBehaviors",
          BindingFlags.Public | BindingFlags.Instance,
          null, Type.EmptyTypes, null);

        if (remove_all == null)
        {
          Logger.LogError($"BehaviorInterface.RemoveAllBehaviors: could not find 'RemoveAllBehaviors()' on type '{parent_type.FullName}'.");
          return;
        }

        remove_all.Invoke(parent, null);
        Logger.LogDebug($"BehaviorInterface.RemoveAllBehaviors: removed all behaviors from object of type '{parent_type.FullName}'.");
      }
      catch (Exception e)
      {
        Host.HandleException(e);
      }
    }

    /// <summary>
    /// Checks if the parent managed object has a behavior of the given type name
    /// by invoking "HasBehavior(string)" via reflection.
    /// </summary>
    [UnmanagedCallersOnly]
    internal static unsafe NativeBool32 HasBehavior(IntPtr parent_handle, NativeString behavior_type_name)
    {
      try
      {
        string? type_name = behavior_type_name;
        if (string.IsNullOrEmpty(type_name))
        {
          Logger.LogError("BehaviorInterface.HasBehavior: behavior type name is null or empty.");
          return false;
        }

        object? parent = GCHandle.FromIntPtr(parent_handle).Target;
        if (parent == null)
        {
          Logger.LogError("BehaviorInterface.HasBehavior: parent object handle is null.");
          return false;
        }

        Type parent_type = parent.GetType();
        MethodInfo? has_method = parent_type.GetMethod("HasBehavior",
          BindingFlags.Public | BindingFlags.Instance,
          null, new Type[] { typeof(string) }, null);

        if (has_method == null)
        {
          Logger.LogError($"BehaviorInterface.HasBehavior: could not find 'HasBehavior(string)' on type '{parent_type.FullName}'.");
          return false;
        }

        object? result = has_method.Invoke(parent, new object[] { type_name });
        return result is bool b && b;
      }
      catch (Exception e)
      {
        Host.HandleException(e);
        return false;
      }
    }

    /// <summary>
    /// Returns the number of behaviors attached to the parent managed object
    /// by reading the "BehaviorCount" property via reflection.
    /// </summary>
    [UnmanagedCallersOnly]
    internal static unsafe Int32 GetBehaviorCount(IntPtr parent_handle)
    {
      try
      {
        object? parent = GCHandle.FromIntPtr(parent_handle).Target;
        if (parent == null)
        {
          Logger.LogError("BehaviorInterface.GetBehaviorCount: parent object handle is null.");
          return 0;
        }

        Type parent_type = parent.GetType();
        PropertyInfo? count_prop = parent_type.GetProperty("BehaviorCount",
          BindingFlags.Public | BindingFlags.Instance);

        if (count_prop == null)
        {
          Logger.LogError($"BehaviorInterface.GetBehaviorCount: could not find 'BehaviorCount' property on type '{parent_type.FullName}'.");
          return 0;
        }

        object? result = count_prop.GetValue(parent);
        return result is int count ? count : 0;
      }
      catch (Exception e)
      {
        Host.HandleException(e);
        return 0;
      }
    }

    /// <summary>
    /// Returns the type names of all behaviors attached to the parent managed 
    /// object by invoking "GetBehaviorTypeNames()" via reflection.
    /// Writes names into a pre-allocated native string array.
    /// </summary>
    [UnmanagedCallersOnly]
    internal static unsafe void GetBehaviorTypeNames(IntPtr parent_handle, NativeString* out_names, Int32* out_count, Int32 max_count)
    {
      try
      {
        object? parent = GCHandle.FromIntPtr(parent_handle).Target;
        if (parent == null)
        {
          Logger.LogError("BehaviorInterface.GetBehaviorTypeNames: parent object handle is null.");
          if (out_count != null) *out_count = 0;
          return;
        }

        Type parent_type = parent.GetType();
        MethodInfo? get_names = parent_type.GetMethod("GetBehaviorTypeNames",
          BindingFlags.Public | BindingFlags.Instance,
          null, Type.EmptyTypes, null);

        if (get_names == null)
        {
          Logger.LogError($"BehaviorInterface.GetBehaviorTypeNames: could not find 'GetBehaviorTypeNames()' on type '{parent_type.FullName}'.");
          if (out_count != null) *out_count = 0;
          return;
        }

        object? result = get_names.Invoke(parent, null);
        if (result is not string[] names)
        {
          if (out_count != null) *out_count = 0;
          return;
        }

        Int32 count = Math.Min(names.Length, max_count);
        if (out_count != null) *out_count = count;

        if (out_names != null)
        {
          for (Int32 i = 0; i < count; i++)
          {
            out_names[i] = new NativeString(names[i]);
          }
        }
      }
      catch (Exception e)
      {
        Host.HandleException(e);
        if (out_count != null) *out_count = 0;
      }
    }

    [UnmanagedCallersOnly]
    internal static unsafe void DestroyBehaviorHandle(IntPtr behavior_handle)
    {
      try
      {
        if (behavior_handle == IntPtr.Zero)
        {
          return;
        }

        GCHandle handle = GCHandle.FromIntPtr(behavior_handle);
        if (handle.IsAllocated)
        {
          handle.Free();
        }
      }
      catch (Exception e)
      {
        Host.HandleException(e);
      }
    }

    private static Type? ResolveType(string type_name)
    {
      // first try the TypeInterface resolver (handles assembly-qualified names)
      Type? type = TypeInterface.FindType(type_name);
      if (type != null)
      {
        return type;
      }

      // fall back to scanning all loaded assemblies
      foreach (var assembly in AssemblyLoader.GetFullLoadedAssemblyContext())
      {
        // try exact full name match
        type = assembly.GetType(type_name, throwOnError: false, ignoreCase: false);
        if (type != null)
        {
          return type;
        }
      }

      // fall back to short name search across all assemblies
      foreach (var assembly in AssemblyLoader.GetFullLoadedAssemblyContext())
      {
        Type[] types;
        try
        {
          types = assembly.GetTypes();
        }
        catch (ReflectionTypeLoadException ex)
        {
          types = ex.Types!;
        }

        foreach (var t in types)
        {
          if (t != null && t.Name == type_name)
          {
            return t;
          }
        }
      }

      return null;
    }

    private static MethodInfo? FindAddBehaviorMethod(Type parent_type, Type behavior_type)
    {
      Type? current = parent_type;
      while (current != null)
      {
        MethodInfo[] methods = current.GetMethods(BindingFlags.Public | BindingFlags.Instance | BindingFlags.DeclaredOnly);
        foreach (MethodInfo method in methods)
        {
          if (method.Name != "AddBehavior")
          {
            continue;
          }

          ParameterInfo[] parameters = method.GetParameters();
          if (parameters.Length != 1)
          {
            continue;
          }

          if (parameters[0].ParameterType.IsAssignableFrom(behavior_type))
          {
            return method;
          }
        }

        current = current.BaseType;
      }

      return null;
    }
  }
}
#nullable disable