using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;
using System.Reflection;
using System.Text;

#nullable enable
namespace OtherCsBindings
{
  [InteropBinding("ManagedObject")]
  internal static class ManagedObject
  {
    public readonly struct MethodKey : IEquatable<MethodKey>
    {
      public readonly string type_name;
      public readonly string name;
      public readonly ManagedType[] arg_types;
      public readonly Int32 param_count;

      public MethodKey(string type_name, string name, ManagedType[] arg_types, Int32 param_count)
      {
        this.type_name = type_name;
        this.name = name;
        this.arg_types = arg_types;
        this.param_count = arg_types.Length;
      }

      public override bool Equals([NotNullWhen(true)] object? obj) => obj is MethodKey key && Equals(key);

      bool IEquatable<MethodKey>.Equals(MethodKey other)
      {
        if (type_name != other.type_name || name != other.name || param_count != other.param_count)
        {
          return false;
        }

        for (Int32 i = 0; i < param_count; i++)
        {
          if (arg_types[i] != other.arg_types[i])
          {
            return false;
          }
        }

        return true;
      }

      public override int GetHashCode()
      {
        return base.GetHashCode();
      }
    }

    internal static Dictionary<MethodKey, MethodInfo> methods = new Dictionary<MethodKey, MethodInfo>();

    private static unsafe MethodInfo? TryGetMethodInfo(Type type, string? name, ManagedType* types, Int32 count, BindingFlags flags)
    {
      MethodInfo? minfo = null;

      ManagedType[]? param_types = new ManagedType[count];

      unsafe
      {
        fixed (ManagedType* param_types_ptr = param_types)
        {
          ulong size = sizeof(ManagedType) * (ulong)count;
          Buffer.MemoryCopy(types, param_types_ptr, size, size);
        }
      }

      MethodKey mkey = new MethodKey(type.FullName!, name!, param_types, count);

      if (methods.TryGetValue(mkey, out minfo))
      {
        if (minfo != null)
        {
          return minfo;
        }
      }

      List<MethodInfo> method_info_list = new List<MethodInfo>();
      method_info_list.AddRange(type.GetMethods(flags));

      Type? base_type = type.BaseType;
      while (base_type != null)
      {
        method_info_list.AddRange(base_type.GetMethods(flags));
        base_type = base_type.BaseType;
      }

      minfo = TypeInterface.FindSuitableMethod<MethodInfo>(name, types, count, CollectionsMarshal.AsSpan(method_info_list));
      if (minfo != null)
      {
        methods.Add(mkey, minfo!);
        return minfo;
      }
      else
      {
        Logger.LogError($"Method '{type.FullName}.{name}[{count}]' not found in type '{type.FullName}' with flags '{flags}'.");
        return null;
      }
    }

    [UnmanagedCallersOnly]
    private static void ClearMethods() => methods.Clear();

    [UnmanagedCallersOnly]
    private static unsafe IntPtr CreateObject(Int32 typeid, NativeBool32 weak_ref, IntPtr parameters, ManagedType* param_types, Int32 count)
    {
      try
      {
        if (!TypeInterface.cached_types.TryGet(typeid, out var type))
        {
          Logger.LogError($"Type with ID '{typeid}' not found in cache.");
          return IntPtr.Zero;
        }
        if (type == null)
        {
          Logger.LogError($"Type with ID '{typeid}' is null.");
          return IntPtr.Zero;
        }

        ReadOnlySpan<ConstructorInfo> ctors = type.GetConstructors(BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance);
        ConstructorInfo? ctor = TypeInterface.FindSuitableMethod(".ctor", param_types, count, ctors);
        if (ctor == null)
        {
          Logger.LogError($"No suitable constructor found for type '{type.FullName}'.");
          return IntPtr.Zero;
        }

        List<MethodInfo> method_info_list = new List<MethodInfo>();
        method_info_list.AddRange(type.GetMethods(BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance));
        // StringBuilder sb = new StringBuilder();
        // sb.Append($"Creating Object of Type '{type.FullName}'\n");
        // sb.Append($"	> Methods for type '{type.FullName}':\n");
        // for (Int32 i = 0; i < method_info_list.Count; i++)
        // {
        //   sb.Append($"  ----  Method '{type.FullName}.{method_info_list[i].Name}[{method_info_list[i].GetParameters().Length}]'\n");
        // }
        // Logger.LogTrace(sb.ToString());

        /// this will be null of count == 0, which is fine (parameters will be null as well)
        object?[]? marshalled_parameters = OtherMemory.MarshalParameterArray(parameters, count, ctor);
        object? result = null;

        if (marshalled_parameters == null)
        {
          result = TypeInterface.CreateInstance(type);
        }
        else
        {
          result = TypeInterface.CreateInstance(type, marshalled_parameters);
          ctor.Invoke(result, marshalled_parameters);
        }

        if (result == null)
        {
          Logger.LogError($"Failed to create instance of type '{type.FullName}'.");
          return IntPtr.Zero;
        }

        var handle = GCHandle.Alloc(result, GCHandleType.Normal);
        AssemblyLoader.RegisterHandle(type.Assembly, handle);
        return GCHandle.ToIntPtr(handle);
      }
      catch (Exception e)
      {
        Host.HandleException(e);
        return IntPtr.Zero;
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe void DestroyObject(IntPtr handle)
    {
      try
      {
        AssemblyLoader.UnregisterHandle(handle);
        GCHandle.FromIntPtr(handle).Free();
      }
      catch (Exception e)
      {
        Host.HandleException(e);
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe void InvokeMethod(IntPtr handle, NativeString method_name, IntPtr parameters, ManagedType* param_types, int count)
    {
      try
      {
        if (method_name == null)
        {
          throw new ArgumentNullException($"{nameof(method_name)} cannot be null.");
        }

        var target = GCHandle.FromIntPtr(handle).Target;
        if (target == null)
        {
          throw new NullReferenceException($"Target object for invoking method [{method_name}]({count}) is null.");
        }

        Type target_type = target.GetType();
        MethodInfo? minfo = TryGetMethodInfo(target_type, method_name, param_types, count, BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Static | BindingFlags.Instance);
        if (minfo == null)
        {
          throw new MissingMethodException($"Method '{target_type.FullName}.{method_name}[{count}]' not found.");
        }

        var marshalled_parameters = OtherMemory.MarshalParameterArray(parameters, count, minfo);
        minfo.Invoke(target, marshalled_parameters);
      }
      catch (Exception ex)
      {
        Host.HandleException(ex);
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe void InvokeMethodRet(IntPtr handle, NativeString name, IntPtr parameters, ManagedType* param_types, Int32 count, IntPtr res)
    {
      try
      {
        var target = GCHandle.FromIntPtr(handle).Target;

        if (target == null)
        {
          Logger.LogError($"Target object for invoking method [{name}]({count}) is null.");
          return;
        }

        if (name == null)
        {
          Logger.LogError($"{nameof(name)} cannot be null.");
          return;
        }

        var target_type = target.GetType();
        var method_info = TryGetMethodInfo(target_type, name, param_types, count, BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Static | BindingFlags.Instance);
        if (method_info == null)
        {
          Logger.LogError($"Method ['{target_type.Name}.{name}'] was not found");
          return;
        }

        var marshalled_parameters = OtherMemory.MarshalParameterArray(parameters, count, method_info);
        object? value = method_info.Invoke(target, marshalled_parameters);
        if (value == null)
        {
          return;
        }

        OtherMemory.MarshalReturn(value, method_info.ReturnType, res);
      }
      catch (Exception e)
      {
        Host.HandleException(e);
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe void InvokeStaticMethod(NativeString type_name, NativeString method_name, IntPtr parameters, ManagedType* param_types, int count)
    {
      try
      {
        if (method_name == null)
        {
          throw new ArgumentNullException($"{nameof(method_name)} cannot be null.");
        }

        Type target_type = Type.GetType(type_name!, throwOnError: true)!;
        MethodInfo? minfo = TryGetMethodInfo(target_type, method_name, param_types, count, BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Static);
        if (minfo == null)
        {
          throw new MissingMethodException($"Method '{target_type.FullName}.{method_name}[{count}]' not found.");
        }

        var marshalled_parameters = OtherMemory.MarshalParameterArray(parameters, count, minfo);
        minfo.Invoke(null, marshalled_parameters);
      }
      catch (Exception ex)
      {
        Host.HandleException(ex);
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe void InvokeStaticMethodRet(NativeString type_name, NativeString name, IntPtr parameters, ManagedType* param_types, Int32 count, IntPtr res)
    {
      try
      {
        if (name == null)
        {
          Logger.LogError($"{nameof(name)} cannot be null.");
          return;
        }

        string full_type_name = type_name.ToString() ?? string.Empty;
        if (string.IsNullOrEmpty(full_type_name))
        {
          Logger.LogError($"{nameof(type_name)} cannot be null or empty.");
          return;
        }

        Type? target_type = TypeInterface.GetType(full_type_name);
        if (target_type == null)
        {
          throw new TypeLoadException($"Type '{full_type_name}' could not be found.");
        }
        var method_info = TryGetMethodInfo(target_type, name, param_types, count, BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Static);
        if (method_info == null)
        {
          Logger.LogError($"Method ['{target_type.Name}.{name}'] was not found");
          return;
        }

        var marshalled_parameters = OtherMemory.MarshalParameterArray(parameters, count, method_info);
        object? value = method_info.Invoke(null, marshalled_parameters);
        if (value == null)
        {
          return;
        }

        OtherMemory.MarshalReturn(value, method_info.ReturnType, res);
      }
      catch (Exception e)
      {
        Host.HandleException(e);
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe NativeBool32 IsFieldPrivate(IntPtr target, NativeString name)
    {
      try
      {
        var obj = GCHandle.FromIntPtr(target).Target;
        if (obj == null)
        {
          Logger.LogError("Target object is null.");
          return false;
        }

        var type = obj.GetType();
        var field = type.GetField(name!, BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance);
        if (field == null)
        {
          Logger.LogError($"Field '{name}' not found in type '{type.FullName}'.");
          return false;
        }

        return field.IsPrivate;
      }
      catch (Exception e)
      {
        Host.HandleException(e);
        return false;
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe void SetField(IntPtr target, NativeString name, IntPtr value)
    {
      try
      {
        var obj = GCHandle.FromIntPtr(target).Target;
        if (obj == null)
        {
          Logger.LogError("Target object is null.");
          return;
        }

        var type = obj.GetType();
        var field = type.GetField(name!, BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance);
        if (field == null)
        {
          Logger.LogError($"Field '{name}' not found in type '{type.FullName}'.");
          return;
        }

        var marshalled_value = OtherMemory.MarshalPointer(value, field.FieldType);
        field.SetValue(obj, marshalled_value);
      }
      catch (Exception e)
      {
        Host.HandleException(e);
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe void GetField(IntPtr target, NativeString name, IntPtr res)
    {
      try
      {
        var obj = GCHandle.FromIntPtr(target).Target;
        if (obj == null)
        {
          throw new NullReferenceException("Target object is null.");
        }

        var type = obj.GetType();
        var field = type.GetField(name!, BindingFlags.Public | BindingFlags.Instance);
        if (field == null)
        {
          throw new MissingMemberException($"Field '{name}' not found in type '{type.FullName}'.");
        }

        var value = field.GetValue(obj);
        OtherMemory.MarshalReturn(value, field.FieldType, res);
      }
      catch (Exception e)
      {
        Logger.LogError($"Error occurred while getting field '{name}' from target object: {e.Message}");
        Host.HandleException(e);
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe void SetProperty(IntPtr target, NativeString name, IntPtr value)
    {
      try
      {
        var obj = GCHandle.FromIntPtr(target).Target;
        if (obj == null)
        {
          throw new NullReferenceException("Target object is null.");
        }

        var type = obj.GetType();
        var prop = type.GetProperty(name!, BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance);
        if (prop == null)
        {
          throw new MissingMemberException($"Property '{name}' not found in type '{type.FullName}'.");
        }

        var marshalled_value = OtherMemory.MarshalPointer(value, prop.PropertyType);
        prop.SetValue(obj, marshalled_value);
      }
      catch (Exception e)
      {
        Host.HandleException(e);
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe void GetProperty(IntPtr target, NativeString name, IntPtr res)
    {
      try
      {
        var obj = GCHandle.FromIntPtr(target).Target;
        if (obj == null)
        {
          throw new NullReferenceException("Target object is null.");
        }

        var type = obj.GetType();
        var prop = type.GetProperty(name!, BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance);
        if (prop == null)
        {
          throw new MissingMemberException($"Property '{name}' not found in type '{type.FullName}'.");
        }

        var value = prop.GetValue(obj);
        OtherMemory.MarshalReturn(value, prop.PropertyType, res);
      }
      catch (Exception e)
      {
        Host.HandleException(e);
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe void SetStringField(IntPtr target, NativeString name, NativeString* value)
    {
      try
      {
        if (value == null)
        {
          throw new ArgumentNullException(nameof(value), "Value pointer cannot be null.");
        }

        var obj = GCHandle.FromIntPtr(target).Target;
        if (obj == null)
        {
          throw new NullReferenceException("Target object is null.");
        }

        var type = obj.GetType();
        var field = type.GetField(name!, BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance);
        if (field == null)
        {
          throw new MissingMemberException($"Field '{name}' not found in type '{type.FullName}'.");
        }

        if (field.FieldType != typeof(string))
        {
          throw new InvalidOperationException($"Field '{name}' is not a string.");
        }

        field.SetValue(obj, value->ToString());
      }
      catch (Exception e)
      {
        Host.HandleException(e);
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe void GetStringField(IntPtr target, NativeString name, NativeString* res)
    {
      try
      {
        if (res == null)
        {
          throw new ArgumentNullException(nameof(res), "Result pointer cannot be null.");
        }

        var obj = GCHandle.FromIntPtr(target).Target;
        if (obj == null)
        {
          throw new NullReferenceException("Target object is null.");
        }

        var type = obj.GetType();
        var field = type.GetField(name!, BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance);
        if (field == null)
        {
          throw new MissingMemberException($"Field '{name}' not found in type '{type.FullName}'.");
        }

        if (field.FieldType != typeof(string))
        {
          throw new InvalidOperationException($"Field '{name}' is not a string.");
        }

        var value = (string?)field.GetValue(obj);
        res->Assign(value!);
      }
      catch (Exception e)
      {
        Host.HandleException(e);
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe void SetStringProperty(IntPtr target, NativeString name, NativeString* value)
    {
      try
      {
        if (value == null)
        {
          throw new ArgumentNullException(nameof(value), "Value pointer cannot be null.");
        }

        var obj = GCHandle.FromIntPtr(target).Target;
        if (obj == null)
        {
          throw new NullReferenceException("Target object is null.");
        }

        var type = obj.GetType();
        var property = type.GetProperty(name!, BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance);
        if (property == null)
        {
          throw new MissingMemberException($"Property '{name}' not found in type '{type.FullName}'.");
        }

        if (property.PropertyType != typeof(string))
        {
          throw new InvalidOperationException($"Property '{name}' is not a string.");
        }

        property.SetValue(obj, value->ToString());
      }
      catch (Exception e)
      {
        Host.HandleException(e);
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe void GetStringProperty(IntPtr target, NativeString name, NativeString* res)
    {
      try
      {
        if (res == null)
        {
          throw new ArgumentNullException(nameof(res), "Result pointer cannot be null.");
        }

        var obj = GCHandle.FromIntPtr(target).Target;
        if (obj == null)
        {
          throw new NullReferenceException("Target object is null.");
        }

        var type = obj.GetType();
        var prop = type.GetProperty(name!, BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance);
        if (prop == null)
        {
          throw new MissingMemberException($"Property '{name}' not found in type '{type.FullName}'.");
        }

        if (prop.PropertyType != typeof(string))
        {
          throw new InvalidOperationException($"Property '{name}' is not a string.");
        }

        var value = (string?)prop.GetValue(obj);
        res->Assign(value!);
      }
      catch (Exception e)
      {
        Host.HandleException(e);
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe UInt64 GetStringFieldLength(IntPtr target, NativeString name)
    {
      try
      {
        var obj = GCHandle.FromIntPtr(target).Target;
        if (obj == null)
        {
          throw new NullReferenceException("Target object is null.");
        }

        var type = obj.GetType();
        var field = type.GetField(name!, BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance);
        if (field == null)
        {
          throw new MissingMemberException($"Field '{name}' not found in type '{type.FullName}'.");
        }

        if (field.FieldType != typeof(string))
        {
          throw new InvalidOperationException($"Field '{name}' is not a string.");
        }

        var value = (string?)field.GetValue(obj);
        return (UInt64)(value?.Length ?? 0);
      }
      catch (Exception e)
      {
        Host.HandleException(e);
        return 0;
      }
    }
    
    [UnmanagedCallersOnly]
    private static unsafe UInt64 GetStringPropertyLength(IntPtr target, NativeString name)
    {
      try
      {
        var obj = GCHandle.FromIntPtr(target).Target;
        if (obj == null)
        {
          throw new NullReferenceException("Target object is null.");
        }

        var type = obj.GetType();
        var property = type.GetProperty(name!, BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance);
        if (property == null)
        {
          throw new MissingMemberException($"Field '{name}' not found in type '{type.FullName}'.");
        }

        if (property.PropertyType != typeof(string))
        {
          throw new InvalidOperationException($"Field '{name}' is not a string.");
        }

        var value = (string?)property.GetValue(obj);
        return (UInt64)(value?.Length ?? 0);
      }
      catch (Exception e)
      {
        Host.HandleException(e);
        return 0;
      }
    }
  }
}
#nullable disable