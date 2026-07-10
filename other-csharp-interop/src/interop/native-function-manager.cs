using System;
using System.Runtime.InteropServices;
using System.Reflection;
using System.Linq;
using System.Collections.Generic;

#nullable enable
namespace OtherCsBindings
{
  [InteropBinding("NativeFunctionManager")]
  internal static class NativeFunctionManager
  {
    struct BindingPoint
    {
      public FieldInfo field;
      public string managed_name;
      public string registered_name;

      public bool autobind;

      public void SetValue(object? obj, IntPtr value)
      {
        if (autobind)
        {
          field.SetValue(obj, value);
        }
      }
    }

    /// field name -> BindingPoint
    private static Dictionary<string, BindingPoint> binding_points = new Dictionary<string, BindingPoint>();

    public static void DiscoverAndRegisterFunctions()
    {
      var assemblies = AssemblyLoader.GetFullLoadedAssemblyContext();
      foreach (var assembly in assemblies)
      {
        DiscoverAssemblyBindingPoints(assembly);
      }
    }

    public static void ClearBindingPoints()
    {
      binding_points.Clear();
    }

    private static void DiscoverAssemblyBindingPoints(Assembly assembly)
    {
      try
      {
        var types = assembly.GetTypes();
        foreach (var type in types)
        {
          DiscoverTypeBindingPoints(type);
        }
      }
      catch (Exception ex)
      {
        Host.HandleException(ex);
      }
    }

    private static void DiscoverTypeBindingPoints(Type type)
    {
      try
      {
        var count = 0;
        var binding_flags = BindingFlags.Static | BindingFlags.NonPublic | BindingFlags.Public;
        var fields = type.GetFields(binding_flags);
        foreach (var field in fields)
        {
          var attr = field.GetCustomAttribute<NativeFunctionAttribute>();
          if (attr == null)
          {
            continue;
          }

          if (binding_points.ContainsKey(attr.Name))
          {
            continue;
          }

          if (!IsNativeFunctionType(field.FieldType))
          {
            Logger.LogError($"Field '{type.FullName}.{field.Name}' has NativeFunctionAttribute but is not a NativeFunction<T> or function pointer");
            continue;
          }

          binding_points.Add(attr.Name, new BindingPoint
          {
            field = field,
            managed_name = field.Name,
            registered_name = attr.Name,
            autobind = attr.AutoBind
          });
          count++;

          Logger.LogDebug($"Discovered native function binding point: {type.FullName}.{field.Name} for native function '{attr.Name}'");
        }
      }
      catch (Exception ex)
      {
        Host.HandleException(ex);
      }
    }

    /// does this work in all cases? this seems fragile
    private static bool IsNativeFunctionType(Type type)
    {
      if (type.IsFunctionPointer)
      {
        return true;
      }

      return false;
    }

    [UnmanagedCallersOnly]
    private static void RediscoverBindingPoints() => DiscoverAndRegisterFunctions();
    [UnmanagedCallersOnly]
    private static void CleanupBindingPoints() => ClearBindingPoints();

    [UnmanagedCallersOnly]
    private static void BindNativeFunction(NativeString name_str, IntPtr target)
    {
      try
      {
        var name = name_str.ToString()!;
        if (!binding_points.TryGetValue(name, out var binding_point))
        {
          Logger.LogError($"No binding point found for native function '{name}'");
          return;
        }

        binding_point.SetValue(null, target);
      }
      catch (Exception e)
      {
        Host.HandleException(e);
      }
    }

    /// calls to this happen before the discovery of assembly modules and binding points for native functions 
    /// so we do not use the binding points api here

    [UnmanagedCallersOnly]
    private static void RegisterInternalCall(NativeString name_str, IntPtr target)
    {
      try
      {
        var name = name_str.ToString()!;

        var name_start = name.IndexOf('+');
        var name_end = name.IndexOf(",", name_start, StringComparison.CurrentCulture);
        var field_name = name.Substring(name_start + 1, name_end - name_start - 1);
        var containing_type_name = name.Remove(name_start, name_end - name_start);

        var type = TypeInterface.FindType(containing_type_name);
        if (type == null)
        {
          return;
        }

        var binding_flags = BindingFlags.Static | BindingFlags.NonPublic;
        var field = type.GetFields(binding_flags).FirstOrDefault(field => field.Name == field_name);
        if (field == null)
        {
          return;
        }

        var field_type = field.FieldType;
        if (!field.FieldType.IsFunctionPointer)
        {
          return;
        }

        field.SetValue(null, target);
      }
      catch (Exception ex)
      {
        Host.HandleException(ex);
      }
    }

    public static bool BindingPointsValid()
    {
      var all_bound = true;
      foreach (var kvp in binding_points)
      {
        var binding_point = kvp.Value;
        var value = binding_point.field.GetValue(null);
        if (value == null || (value is IntPtr ptr && ptr == IntPtr.Zero))
        {
          Logger.LogError($"Native function '{binding_point.registered_name}' was not bound to managed counterpart.");
          all_bound = false;
        }
      }

      if (all_bound)
      {
        Logger.LogInfo("All native functions successfully bound to managed counterparts.");
      }
      return all_bound;
    }

    [UnmanagedCallersOnly]
    private static NativeBool32 ValidateBindingPoints()
    {
      return BindingPointsValid();
    }
  }
}
#nullable disable