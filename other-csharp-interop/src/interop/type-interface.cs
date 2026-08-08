using System;
using System.Collections.Generic;
using System.Collections.Immutable;
using System.ComponentModel;
using System.Reflection;
using System.Text;
using System.Linq;
using System.Runtime.InteropServices;

#nullable enable
namespace OtherCsBindings
{
  enum InternalValueType : Byte
  {
    /// primitive types
    OEBOOL,
    CHAR,
    INT8,
    INT16,
    INT32,
    INT64,
    UINT8,
    UINT16,
    UINT32,
    UINT64,
    FLOAT,
    DOUBLE,
    STRING,

    /// engine types
    VEC2,
    VEC3,
    VEC4,

    MAT2,
    MAT3,
    MAT4,

    SAMPLER2D,
    SAMPLER2D_ARRAY,

    ASSET,
    ENTITY,

    /// user types
    USER_TYPE,
    OPAQUE_HANDLE,

    /// error/misc
    EMPTY_TYPE,
   };

  [InteropBinding("TypeInterface")]
  internal static class TypeInterface
  {

    internal readonly static Set<Type> cached_types = new();
    internal readonly static Set<MethodInfo> cached_methods = new();
    internal readonly static Set<FieldInfo> cached_fields = new();
    internal readonly static Set<PropertyInfo> cached_properties = new();
    internal readonly static Set<Attribute> cached_attributes = new();

    /// cached reflection objects root their declaring assembly, keeping an unloaded ALC alive
    ///  forever — evict everything belonging to it before the context unloads
    internal static void EvictAssemblyFromCaches(Assembly asm)
    {
      cached_types.RemoveWhere(t => t.Assembly == asm);
      cached_methods.RemoveWhere(m => m.Module.Assembly == asm);
      cached_fields.RemoveWhere(f => f.Module.Assembly == asm);
      cached_properties.RemoveWhere(p => p.Module.Assembly == asm);
      cached_attributes.RemoveWhere(a => a.GetType().Assembly == asm);
    }

    private static Dictionary<Type, ManagedType> type_converters = new()
    {
      { typeof(sbyte), ManagedType.SByte },
      { typeof(byte), ManagedType.Byte },
      { typeof(short), ManagedType.Short },
      { typeof(ushort), ManagedType.UShort },
      { typeof(int), ManagedType.Int },
      { typeof(uint), ManagedType.UInt },
      { typeof(long), ManagedType.Long },
      { typeof(ulong), ManagedType.ULong },
      { typeof(float), ManagedType.Float },
      { typeof(double), ManagedType.Double },
      { typeof(bool), ManagedType.Bool },
      { typeof(NativeBool32), ManagedType.Bool },
      { typeof(string), ManagedType.String },
      { typeof(NativeString), ManagedType.String },
    };

    internal enum TypeAccessibility
    {
      Public,
      Private,
      Protected,
      Internal,
      ProtectedPublic,
      PrivateProtected
    }

    private static TypeAccessibility GetTypeAccessibility(FieldInfo finfo)
    {
      if (finfo.IsPublic) return TypeAccessibility.Public;
      if (finfo.IsPrivate) return TypeAccessibility.Private;
      if (finfo.IsFamily) return TypeAccessibility.Protected;
      if (finfo.IsAssembly) return TypeAccessibility.Internal;
      if (finfo.IsFamilyOrAssembly) return TypeAccessibility.ProtectedPublic;
      if (finfo.IsFamilyAndAssembly) return TypeAccessibility.PrivateProtected;
      return TypeAccessibility.Public;
    }

    private static TypeAccessibility GetTypeAccessibility(MethodInfo minfo)
    {
      if (minfo.IsPublic) return TypeAccessibility.Public;
      if (minfo.IsPrivate) return TypeAccessibility.Private;
      if (minfo.IsFamily) return TypeAccessibility.Protected;
      if (minfo.IsAssembly) return TypeAccessibility.Internal;
      if (minfo.IsFamilyOrAssembly) return TypeAccessibility.ProtectedPublic;
      if (minfo.IsFamilyAndAssembly) return TypeAccessibility.PrivateProtected;
      return TypeAccessibility.Public;
    }

    internal static Type? FindType(string? name)
    {
      var type = Type.GetType(name!,
        (name) => AssemblyLoader.ResolveAssembly(null, name),
        (assembly, name, ignore) =>
        {
          return assembly != null ?
            assembly.GetType(name, false, ignore) : Type.GetType(name, false, ignore);
        }
      );

      if (type == null)
      {
        type = AssemblyLoader.GetNetCoreType(name);
      }

      if (type == null)
      {
        type = cached_types.FirstOrDefault(t => t.FullName == name);
      }

      return type;
    }

    internal static List<Type> GetAllTypesWithAttribute<T>() where T : Attribute
    {
      try 
      {
        List<Type> types_with_attribute = new();

        foreach (var asm in AssemblyLoader.GetFullLoadedAssemblyContext())
        {
          Type[] types;
          try
          {
            types = asm.GetTypes();
          }
          catch (ReflectionTypeLoadException ex)
          {
            types = ex.Types!;
          }

          foreach (var type in types)
          {
            if (type == null)
            {
              continue;
            }

            var attrs = type.GetCustomAttributes(typeof(T), false);
            if (attrs.Length > 0)
            {
              types_with_attribute.Add(type);
            }
          }
        }
        
        return types_with_attribute;
      }
      catch (Exception ex)
      {
        Host.HandleException(ex);
        return new List<Type>();
      }
    }

    internal static List<Type> GetAllTypes()
    {
      try 
      {
        List<Type> all_types = new();

        foreach (var asm in AssemblyLoader.GetFullLoadedAssemblyContext())
        {
          Type[] types;
          try
          {
            types = asm.GetTypes();
          }
          catch (ReflectionTypeLoadException ex)
          {
            types = ex.Types!;
          }

          foreach (var type in types)
          {
            if (type == null)
            {
              continue;
            }

            all_types.Add(type);
          }
        }

        return all_types;
      }
      catch (Exception ex)
      {
        Host.HandleException(ex);
        return new List<Type>();
      }
    }

    internal static Type? GetType(string name)
    {
      try
      {
        Type? type = FindType(name);
        if (type == null)
        {
          Logger.LogError($"Type '{name}' not found.");
          return null;
        }

        return type;
      }
      catch (Exception ex)
      {
        Host.HandleException(ex);
        return null;
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe void GetAssemblyTypes(Int32 asm_id, Int32* out_types, Int32* out_type_count)
    {
      try
      {
        if (!AssemblyLoader.TryGetAssembly(asm_id, out var asm))
        {
          Logger.LogError($"Couldn't get types for assembly '{asm_id}', assembly not found");
          return;
        }

        if (asm == null)
        {
          Logger.LogError($"Couldn't get types for assembly '{asm_id}', assembly is null");
          return;
        }

        ReadOnlySpan<Type> asm_types = asm.GetTypes();
        if (out_type_count != null)
        {
          *out_type_count = asm_types.Length;
        }
        else
        {
          Logger.LogTrace($"Found {asm_types.Length} types in assembly {asm.FullName}");
        }

        if (out_types != null)
        {
          for (Int32 i = 0; i < asm_types.Length; i++)
          {
            Logger.LogTrace($"  > Adding type {asm_types[i].FullName} to cache");
            out_types[i] = cached_types.Add(asm_types[i]);
          }
        }
      }
      catch (Exception ex)
      {
        Host.HandleException(ex);
      }
    }

    internal static unsafe T? FindSuitableMethod<T>(string? method_name, ManagedType* param_types, Int32 argc, ReadOnlySpan<T> methods) where T : MethodBase
    {
      if (method_name == null)
      {
        return null;
      }

      Logger.LogTrace($"Finding suitable method '{method_name}' with {argc} arguments");
      foreach (var minfo in methods)
      {
        // Logger.LogTrace($" > Checking method '{minfo}' ({minfo.GetParameters().Length})");
        ParameterInfo[] parameters = minfo.GetParameters();
        if (parameters.Length != argc)
        {
          continue;
        }
        // Logger.LogTrace($"	> Method '{minfo}' has {parameters.Length} parameters");
        if (method_name == minfo.ToString())
        {
          // Logger.LogTrace($"	> Found exact match for method '{minfo}'");
          return minfo;
        }

        if (minfo.Name != method_name)
        {
          // Logger.LogTrace($"	!> Method '{minfo}' doesn't match the required name");
          continue;
        }

        // Logger.LogTrace($"	> Checking method '{minfo}' for parameter types");
        Int32 type_match = 0;
        for (Int32 i = 0; i < parameters.Length; i++)
        {
          ManagedType ptype;
          if (parameters[i].ParameterType.IsPointer || parameters[i].ParameterType == typeof(IntPtr))
          {
            ptype = ManagedType.Pointer;
          }
          else if (!type_converters.TryGetValue(parameters[i].ParameterType, out ptype))
          {
            ptype = ManagedType.Unknown;
          }

          // Logger.LogTrace($"		> Parameter {i} : {ptype} == {param_types[i]}");
          if (ptype == param_types[i])
          {
            type_match++;
          }
        }

        if (type_match == argc)
        {
          // Logger.LogTrace($"	> Found suitable method '{minfo}' with {type_match} matching parameters");
          return minfo;
        }
        else
        {
          // Logger.LogTrace($"	!> Method '{minfo}' has {type_match} matching parameters, expected {argc}");
        }
      }

      Logger.LogError($"No suitable method '{method_name}' found with {argc} parameters in the provided methods.");
      return null;
    }

    internal static object? CreateInstance(Type type, params object?[]? args)
    {
      try
      {
        return type.Assembly.CreateInstance(type.FullName ?? string.Empty, false,
                                            BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance,
                                            null, args!, null, null);
      }
      catch (Exception e)
      {
        Logger.LogError($"Failed to create instance of type '{type.FullName}': {e.Message}");
        Host.HandleException(e);
        return null;
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe void GetNetCoreTypes(Int32* out_types, Int32* out_type_count)
    {
      try
      {
        if (!AssemblyLoader.CoreAssembliesLoaded)
        {
          Logger.LogError("Core assemblies not loaded, cannot get .NET Core types");
          return;
        }

        ReadOnlySpan<Type> types = AssemblyLoader.CoreTypes;
        if (out_type_count != null)
        {
          *out_type_count = types.Length;
        }
        else
        {
          Logger.LogError($"Found {types.Length} types in .NET Core assemblies");
        }

        if (out_types != null)
        {
          for (Int32 i = 0; i < types.Length; i++)
          {
            out_types[i] = cached_types.Add(types[i]);
          }
        }
      }
      catch (Exception ex)
      {
        Host.HandleException(ex);
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe void GetTypeId(NativeString name, Int32* out_type)
    {
      try
      {
        var type = FindType(name);
        if (type == null)
        {
          Logger.LogError($"Couldn't get type id for '{name}', type not found");
          *out_type = 0;
          return;
        }

        *out_type = cached_types.Add(type);
      }
      catch (Exception ex)
      {
        Host.HandleException(ex);
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe NativeString GetAsmQualifiedName(Int32 type)
    {
      try
      {
        if (!cached_types.TryGet(type, out var t))
        {
          return NativeString.Null();
        }

        return t!.AssemblyQualifiedName;
      }
      catch (Exception e)
      {
        Host.HandleException(e);
        return NativeString.Null();
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe NativeString GetFullTypeName(Int32 type_id)
    {
      try
      {
        if (!cached_types.TryGet(type_id, out var type))
        {
          return NativeString.Null();
        }

        return type!.FullName;
      }
      catch (Exception ex)
      {
        Host.HandleException(ex);
        return NativeString.Null();
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe void GetTypeMethods(Int32 type, Int32* method_arr, Int32* count)
    {
      try
      {
        if (!cached_types.TryGet(type, out var t))
        {
          return;
        }

        ReadOnlySpan<MethodInfo> methods = t!.GetMethods(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static);
        if (methods.Length == 0)
        {
          *count = 0;
          return;
        }

        *count = methods.Length;
        if (method_arr == null)
        {
          return;
        }

        for (Int32 i = 0; i < methods.Length; i++)
        {
          // Logger.LogTrace($"  > Adding method [class : {t.Name}] {methods[i].Name} to cache");
          method_arr[i] = cached_methods.Add(methods[i]);
        }

      }
      catch (Exception ex)
      {
        Host.HandleException(ex);
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe void GetTypeFields(Int32 type, Int32* field_arr, Int32* field_count)
    {
      try
      {
        if (field_count == null)
        {
          throw new ArgumentNullException(nameof(field_count));
        }

        if (!cached_types.TryGet(type, out var t))
        {
          return;
        }

        ReadOnlySpan<FieldInfo> fields = t!.GetFields(BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static);
        if (fields.Length == 0)
        {
          *field_count = 0;
          return;
        }


        *field_count = fields.Length;

        if (field_arr == null)
        {
          return;
        }

        for (Int32 i = 0; i < fields.Length; i++)
        {
          // Logger.LogTrace($"  > Adding field [class : {t.Name}] {fields[i].Name} to cache");
          field_arr[i] = cached_fields.Add(fields[i]);
        }
      }
      catch (Exception ex)
      {
        Host.HandleException(ex);
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe void GetTypeProperties(Int32 type, Int32* arr, Int32* count)
    {
      try
      {
        if (!cached_types.TryGet(type, out var t))
        {
          return;
        }

        ReadOnlySpan<PropertyInfo> properties = t!.GetProperties(BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static);
        if (properties.Length == 0)
        {
          *count = 0;
          return;
        }

        *count = properties.Length;

        if (arr == null)
        {
          return;
        }

        for (Int32 i = 0; i < properties.Length; i++)
        {
          // Logger.LogTrace($"  > Adding property [class : {t.Name}] {properties[i].Name} to cache");
          arr[i] = cached_properties.Add(properties[i]);
        }
      }
      catch (Exception ex)
      {
        Host.HandleException(ex);
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe NativeBool32 HasAttribute(Int32 type, Int32 attr_type)
    {
      try
      {
        if (!cached_types.TryGet(type, out var t) || !cached_attributes.TryGet(attr_type, out var attr))
        {
          return false;
        }

        return t!.GetCustomAttribute(attr) != null;
      }
      catch (Exception ex)
      {
        Host.HandleException(ex);
        return false;
      }
    }

    [UnmanagedCallersOnly]
    private static NativeBool32 IsDerivedFrom(Int32 type, Int32 base_type)
    {
      try
      {
        if (!cached_types.TryGet(type, out var t) || !cached_types.TryGet(base_type, out var b))
        {
          return false;
        }

        return t!.IsSubclassOf(b!);
      }
      catch (Exception ex)
      {
        Host.HandleException(ex);
        return false;
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe void GetAttributes(Int32 type, Int32* attributes, Int32* count)
    {
      try
      {
        if (!cached_types.TryGet(type, out var t))
        {
          return;
        }

        ImmutableArray<object> attrs = t!.GetCustomAttributes(true).ToImmutableArray();
        if (attrs == null || attrs.Length == 0)
        {
          // Logger.LogTrace($"No attributes found for type '{t.FullName}'");
          *count = 0;
          return;
        }

        *count = attrs.Length;
        // Logger.LogTrace($"Found {attrs.Length} attributes for type '{t.FullName}'");

        if (attributes == null)
        {
          return;
        }

        for (Int32 i = 0; i < attrs.Length; i++)
        {
          // Logger.LogTrace($"  > Adding attribute [class : {t.Name}] {attrs[i].GetType().Name} to cache");
          attributes[i] = cached_attributes.Add((Attribute)attrs[i]);
        }
      }
      catch (Exception ex)
      {
        Host.HandleException(ex);
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe NativeBool32 HasMethod(Int32 type, NativeString method_name)
    {
      try
      {
        if (!cached_types.TryGet(type, out var t))
        {
          return false;
        }

        return t!.GetMethod(method_name!, BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static) != null;
      }
      catch (Exception ex)
      {
        Host.HandleException(ex);
        return false;
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe NativeString GetMethodName(Int32 method_info)
    {
      try
      {
        if (!cached_methods.TryGet(method_info, out var minfo))
        {
          return NativeString.Null();
        }

        return minfo!.Name;
      }
      catch (Exception ex)
      {
        Host.HandleException(ex);
        return NativeString.Null();
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe void GetMethodReturnType(Int32 method_info, Int32* out_type)
    {
      try
      {
        if (!cached_methods.TryGet(method_info, out var minfo) || out_type == null)
        {
          return;
        }

        *out_type = cached_types.Add(minfo!.ReturnType);
      }
      catch (Exception ex)
      {
        Host.HandleException(ex);
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe void GetMethodParameterTypes(Int32 minfo, Int32* out_param_types, Int32* count)
    {
      try
      {
        if (!cached_methods.TryGet(minfo, out var method_info))
        {
          return;
        }

        ReadOnlySpan<ParameterInfo> parameters = method_info!.GetParameters();

        if (parameters.Length == 0)
        {
          *count = 0;
          return;
        }

        *count = parameters.Length;

        if (out_param_types == null)
        {
          return;
        }

        for (Int32 i = 0; i < parameters.Length; i++)
        {
          out_param_types[i] = cached_types.Add(parameters[i].ParameterType);
        }
      }
      catch (Exception e)
      {
        Host.HandleException(e);
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe void GetMethodAttributes(Int32 minfo, Int32* out_attrs, Int32* count)
    {
      try
      {
        if (!cached_methods.TryGet(minfo, out var method_info))
        {
          *count = 0;
          return;
        }

        var attributes = method_info!.GetCustomAttributes().ToImmutableArray();

        if (attributes.Length == 0)
        {
          *count = 0;
          return;
        }

        *count = attributes.Length;

        if (out_attrs == null)
        {
          return;
        }

        for (Int32 i = 0; i < attributes.Length; i++)
        {
          out_attrs[i] = cached_attributes.Add(attributes[i]);
        }
      }
      catch (Exception ex)
      {
        Host.HandleException(ex);
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe NativeBool32 IsMethodStatic(Int32 method_info)
    {
      try
      {
        if (!cached_methods.TryGet(method_info, out var minfo))
        {
          return false;
        }

        return minfo!.IsStatic;
      }
      catch (Exception ex)
      {
        Host.HandleException(ex);
        return false;
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe TypeAccessibility GetMethodAccessibility(Int32 id)
    {
      try
      {
        if (!cached_methods.TryGet(id, out var minfo))
        {
          return TypeAccessibility.Internal;
        }

        return GetTypeAccessibility(minfo!);
      }
      catch (Exception ex)
      {
        Host.HandleException(ex);
        return TypeAccessibility.Public;
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe NativeBool32 HasField(Int32 type, NativeString field_name)
    {
      try
      {
        if (!cached_types.TryGet(type, out var t))
        {
          Logger.LogError($"Type with id {type} not found in cache.");
          return false;
        }

        return t!.GetField(field_name!, BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static) != null ||
               t.GetProperty(field_name!, BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static) != null;
      }
      catch (Exception ex)
      {
        Host.HandleException(ex);
        return false;
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe NativeString GetFieldName(Int32 id, Int32* out_type)
    {
      try
      {
        if (!cached_fields.TryGet(id, out var finfo))
        {
          return NativeString.Null();
        }

        return finfo!.Name;
      }
      catch (Exception ex)
      {
        Host.HandleException(ex);
        return NativeString.Null();
      }
    }

    [UnmanagedCallersOnly]
		private static unsafe void GetFieldType(Int32 id, Int32* out_type)
    {
			try
      {
				if (!cached_fields.TryGet(id, out var finfo) || out_type == null)
        {
					return;
				}

				*out_type = cached_types.Add(finfo!.FieldType);
			}
      catch (Exception ex)
      {
        Host.HandleException(ex);
      }
		}

    static bool IsProperty(Int32 id)
    {
      return cached_properties.Contains(id);
    }

    static unsafe void GetPropertyValueType(Int32 id, Byte* out_byte)
    {
      try
      {
        if (!cached_properties.TryGet(id, out var pinfo) || out_byte == null)
        {
          return;
        }

        var t = pinfo!.PropertyType;
        InternalValueType value_type = InternalValueType.EMPTY_TYPE;

        if (t == typeof(bool) || t == typeof(NativeBool32))
        {
          value_type = InternalValueType.OEBOOL;
        }
        else if (t == typeof(char))
        {
          value_type = InternalValueType.CHAR;
        }
        else if (t == typeof(sbyte))
        {
          value_type = InternalValueType.INT8;
        }
        else if (t == typeof(short))
        {
          value_type = InternalValueType.INT16;
        }
        else if (t == typeof(int))
        {
          value_type = InternalValueType.INT32;
        }
        else if (t == typeof(long))
        {
          value_type = InternalValueType.INT64;
        }
        else if (t == typeof(byte))
        {
          value_type = InternalValueType.UINT8;
        }
        else if (t == typeof(ushort))
        {
          value_type = InternalValueType.UINT16;
        }
        else if (t == typeof(uint))
        {
          value_type = InternalValueType.UINT32;
        }
        else if (t == typeof(ulong))
        {
          value_type = InternalValueType.UINT64;
        }
        else if (t == typeof(float))
        {
          value_type = InternalValueType.FLOAT;
        }
        else if (t == typeof(double))
        {
          value_type = InternalValueType.DOUBLE;
        }
        else if (t == typeof(string))
        {
          value_type = InternalValueType.STRING;
        }
        else if (t == typeof(IntPtr) || t.IsPointer)
        {
          value_type = InternalValueType.OPAQUE_HANDLE;
        }
        else if (t.FullName == "OtherEngine.Vec2")
        {
          value_type = InternalValueType.VEC2;
        }
        else if (t.FullName == "OtherEngine.Vec3")
        {
          value_type = InternalValueType.VEC3;
        }
        else if (t.FullName == "OtherEngine.Vec4")
        {
          value_type = InternalValueType.VEC4;
        }
        else if (t.FullName == "OtherEngine.Mat2")
        {
          value_type = InternalValueType.MAT2;
        }
        else if (t.FullName == "OtherEngine.Mat3")
        {
          value_type = InternalValueType.MAT3;
        }
        else if (t.FullName == "OtherEngine.Mat4")
        {
          value_type = InternalValueType.MAT4;
        }
        else if (t.FullName == "OtherEngine.Sampler2D")
        {
          value_type = InternalValueType.SAMPLER2D;
        }
        else if (t.FullName == "OtherEngine.Sampler2DArray")
        {
          value_type = InternalValueType.SAMPLER2D_ARRAY;
        }
        else if (t.FullName == "OtherEngine.Asset")
        {
          value_type = InternalValueType.ASSET;
        }
        else if (t.FullName == "OtherEngine.Entity")
        {
          value_type = InternalValueType.ENTITY;
        }
        else
        {
          value_type = InternalValueType.USER_TYPE;
        }

        *out_byte = (Byte)value_type;
      }
      catch (Exception ex)
      {
        Host.HandleException(ex);
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe void GetFieldValueType(Int32 id, Byte* out_byte) {
      if (IsProperty(id)) {
        GetPropertyValueType(id, out_byte);
        return;
      }

      try
      {
        if (!cached_fields.TryGet(id, out var finfo) || out_byte == null)
        {
          return;
        }

        var t = finfo!.FieldType;
        InternalValueType value_type = InternalValueType.EMPTY_TYPE;

        if (t == typeof(bool) || t == typeof(NativeBool32))
        {
          value_type = InternalValueType.OEBOOL;
        }
        else if (t == typeof(char))
        {
          value_type = InternalValueType.CHAR;
        }
        else if (t == typeof(sbyte))
        {
          value_type = InternalValueType.INT8;
        }
        else if (t == typeof(short))
        {
          value_type = InternalValueType.INT16;
        }
        else if (t == typeof(int))
        {
          value_type = InternalValueType.INT32;
        }
        else if (t == typeof(long))
        {
          value_type = InternalValueType.INT64;
        }
        else if (t == typeof(byte))
        {
          value_type = InternalValueType.UINT8;
        }
        else if (t == typeof(ushort))
        {
          value_type = InternalValueType.UINT16;
        }
        else if (t == typeof(uint))
        {
          value_type = InternalValueType.UINT32;
        }
        else if (t == typeof(ulong))
        {
          value_type = InternalValueType.UINT64;
        }
        else if (t == typeof(float))
        {
          value_type = InternalValueType.FLOAT;
        }
        else if (t == typeof(double))
        {
          value_type = InternalValueType.DOUBLE;
        }
        else if (t == typeof(string))
        {
          value_type = InternalValueType.STRING;
        }
        else if (t == typeof(IntPtr) || t.IsPointer)
        {
          value_type = InternalValueType.OPAQUE_HANDLE;
        }
        else if (t.FullName == "OtherEngine.Vec2")
        {
          value_type = InternalValueType.VEC2;
        }
        else if (t.FullName == "OtherEngine.Vec3")
        {
          value_type = InternalValueType.VEC3;
        }
        else if (t.FullName == "OtherEngine.Vec4")
        {
          value_type = InternalValueType.VEC4;
        }
        else if (t.FullName == "OtherEngine.Mat2")
        {
          value_type = InternalValueType.MAT2;
        }
        else if (t.FullName == "OtherEngine.Mat3")
        {
          value_type = InternalValueType.MAT3;
        }
        else if (t.FullName == "OtherEngine.Mat4")
        {
          value_type = InternalValueType.MAT4;
        }
        else if (t.FullName == "OtherEngine.Sampler2D")
        {
          value_type = InternalValueType.SAMPLER2D;
        }
        else if (t.FullName == "OtherEngine.Sampler2DArray")
        {
          value_type = InternalValueType.SAMPLER2D_ARRAY;
        }
        else if (t.FullName == "OtherEngine.Asset")
        {
          value_type = InternalValueType.ASSET;
        }
        else if (t.FullName == "OtherEngine.Entity")
        {
          value_type = InternalValueType.ENTITY;
        }
        else
        {
          value_type = InternalValueType.USER_TYPE;
        }

        *out_byte = (Byte)value_type;
      }
      catch (Exception ex)
      {
        Host.HandleException(ex);
      }
    }

		[UnmanagedCallersOnly]
		private static unsafe TypeAccessibility GetFieldAccessibility(Int32 id)
    {
			try
      {
				if (!cached_fields.TryGet(id, out var finfo))
        {
					return TypeAccessibility.Internal;
				}

				return GetTypeAccessibility(finfo!);
			}
      catch (Exception ex)
      {
        Host.HandleException(ex);
        return TypeAccessibility.Public;
      }
		}

		[UnmanagedCallersOnly]
		private static unsafe void GetFieldAttributes(Int32 id, Int32* out_attrs, Int32* count)
    {
			try
      {
				if (!cached_fields.TryGet(id, out var finfo))
        {
					*count = 0;
					return;
				}

				var attributes = finfo!.GetCustomAttributes(true).ToImmutableArray();

				if (attributes.Length == 0)
        {
					*count = 0;
					return;
				}

				*count = attributes.Length;

				if (out_attrs == null)
        {
					return;
				}

				for (Int32 i = 0; i < attributes.Length; i++)
        {
					out_attrs[i] = cached_attributes.Add((Attribute)attributes[i]);
				}
			}
      catch (Exception ex)
      {
        Host.HandleException(ex);
      }
		}

    [UnmanagedCallersOnly]
    private static unsafe void GetDefaultValue(Int32 id, IntPtr out_val)
    {
      try
      {
        if (IsProperty(id))
        {
          if (!cached_properties.TryGet(id, out var pinfo))
          {
            Logger.LogError($"Property with ID {id} not found in cache.");
            return;
          }

          var default_value_attr = pinfo!.GetCustomAttribute<DefaultValueAttribute>();
          if (default_value_attr == null)
          {
            Logger.LogDebug($"No DefaultValueAttribute found on property '{pinfo!.Name}'.");
            return;
          }

          var value = default_value_attr.Value;
          if (value == null)
          {
            Logger.LogDebug($"Default value for property '{pinfo!.Name}' is null.");
            return;
          }

          OtherMemory.MarshalReturn(value, value.GetType(), out_val);
        }
        else
        {
          if (!cached_fields.TryGet(id, out var finfo))
          {
            Logger.LogError($"Field with ID {id} not found in cache.");
            return;
          }

          var default_value_attr = finfo!.GetCustomAttribute<DefaultValueAttribute>();
          if (default_value_attr == null)
          {
            return;
          }

          var value = default_value_attr.Value;
          if (value == null)
          {
            Logger.LogDebug($"Default value for field '{finfo!.Name}' is null.");
            return;
          }

          OtherMemory.MarshalReturn(value, value.GetType(), out_val);
        }
      }
      catch (Exception ex)
      {
        Host.HandleException(ex);
      }
    }

    [UnmanagedCallersOnly]
		private static unsafe NativeString GetPropertyName(Int32 id, Int32* out_type)
    {
			try
      {
				if (!cached_properties.TryGet(id, out var pinfo))
        {
					return NativeString.Null();
				}

				return pinfo!.Name;
			}
      catch (Exception ex)
      {
        Host.HandleException(ex);
        return NativeString.Null();
      }
		}

		[UnmanagedCallersOnly]
		private static unsafe void GetPropertyType(Int32 id, Int32* out_type)
    {
			try 
      {
				if (!cached_properties.TryGet(id, out var pinfo) || out_type == null)
        {
					return;
				}

				*out_type = cached_types.Add(pinfo!.PropertyType);
			}
      catch (Exception ex)
      {
        Host.HandleException(ex);
      }
		}

		[UnmanagedCallersOnly]
		private static unsafe void GetPropertyAttributes(Int32 id, Int32* out_attrs, Int32* count)
    {
			try
      {
				if (!cached_properties.TryGet(id, out var pinfo))
        {
					*count = 0;
					return;
				}

				var attributes = pinfo!.GetCustomAttributes().ToImmutableArray();

				if (attributes.Length == 0)
        {
					*count = 0;
					return;
				}

				*count = attributes.Length;

				if (out_attrs == null)
        {
					return;
				}

				for (Int32 i = 0; i < attributes.Length; i++)
        {
					out_attrs[i] = cached_attributes.Add(attributes[i]);
				}
			}
      catch (Exception ex)
      {
        Host.HandleException(ex);
      }
		}

		[UnmanagedCallersOnly]
		private static unsafe void GetAttributeType(Int32 attr, Int32* out_type)
    {
      try
      {
        if (!cached_attributes.TryGet(attr, out var attribute) || out_type == null)
        {
          return;
        }

        Type attr_type = attribute!.GetType();
        if (attr_type == null)
        {
          Logger.LogError($"Attribute type for ID {attr} is null.");
          *out_type = -1;
          return;
        }
        *out_type = cached_types.Add(attr_type);
			}
      catch (Exception e)
      {
        Host.HandleException(e);
      }
		}

    [UnmanagedCallersOnly]
		private static unsafe void GetAttributeValue(Int32 attr, NativeString field, IntPtr out_val)
    {
      try
      {
        if (!cached_attributes.TryGet(attr, out var attribute))
        {
          Logger.LogError($"Attribute with ID {attr} not found in cache.");
          return;
        }

        var target = attribute!.GetType();
        var f = target!.GetField(field!, BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance);
        if (f == null)
        {
          Logger.LogError($"Field '{field}' not found in attribute type '{target.FullName}'.");
          return;
        }

        var value = f.GetValue(attribute);
        if (value == null)
        {
          Logger.LogError($"Field '{f.Name}' in attribute '{target.FullName}' is null.");
          return;
        }
        
        OtherMemory.MarshalReturn(value, value.GetType(), out_val);
      }
      catch (Exception ex)
      {
        Host.HandleException(ex);
      }
		}

    internal static string DumpTypeReflectionData(Type type)
    {
      if (type == null)
      {
        return "Type is null";
      }

      var sb = new StringBuilder();

      // Basic type information
      sb.AppendLine($"Type: {type.FullName}");
      sb.AppendLine($"Assembly: {type.Assembly.FullName}");
      sb.AppendLine($"Namespace: {type.Namespace}");
      sb.AppendLine($"Is Generic: {type.IsGenericType}");
      sb.AppendLine($"Is Abstract: {type.IsAbstract}");
      sb.AppendLine($"Is Sealed: {type.IsSealed}");
      sb.AppendLine($"Is Class: {type.IsClass}");
      sb.AppendLine($"Is Interface: {type.IsInterface}");
      sb.AppendLine($"Is Enum: {type.IsEnum}");
      sb.AppendLine($"Is Value Type: {type.IsValueType}");
      sb.AppendLine($"Base Type: {type.BaseType?.FullName ?? "None"}");
      sb.AppendLine();

      // Interfaces
      var interfaces = type.GetInterfaces();
      sb.AppendLine($"Interfaces ({interfaces.Length}):");
      foreach (var iface in interfaces)
      {
        sb.AppendLine($"  - {iface.FullName}");
      }
      sb.AppendLine();

      // Generic parameters
      if (type.IsGenericType)
      {
        var genericArgs = type.GetGenericArguments();
        sb.AppendLine($"Generic Arguments ({genericArgs.Length}):");
        foreach (var arg in genericArgs)
        {
          sb.AppendLine($"  - {arg.FullName ?? arg.Name}");
        }
        sb.AppendLine();
      }

      // Fields
      var fields = type.GetFields(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static);
      sb.AppendLine($"Fields ({fields.Length}):");
      foreach (var field in fields)
      {
        var accessibility = GetTypeAccessibility(field);
        var modifiers = new List<string>();
        if (field.IsStatic) modifiers.Add("static");
        if (field.IsInitOnly) modifiers.Add("readonly");
        if (field.IsLiteral) modifiers.Add("const");

        var modifierStr = modifiers.Count > 0 ? string.Join(" ", modifiers) + " " : "";
        sb.AppendLine($"  {accessibility.ToString().ToLower()} {modifierStr}{field.FieldType.Name} {field.Name}");
      }
      sb.AppendLine();

      // Properties
      var properties = type.GetProperties(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static);
      sb.AppendLine($"Properties ({properties.Length}):");
      foreach (var prop in properties)
      {
        var getter = prop.GetGetMethod(true);
        var setter = prop.GetSetMethod(true);
        var getterAccess = getter != null ? GetTypeAccessibility(getter).ToString().ToLower() : "none";
        var setterAccess = setter != null ? GetTypeAccessibility(setter).ToString().ToLower() : "none";

        sb.AppendLine($"  {prop.PropertyType.Name} {prop.Name} {{ get: {getterAccess}, set: {setterAccess} }}");
      }
      sb.AppendLine();

      // Methods
      var methods = type.GetMethods(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static);
      sb.AppendLine($"Methods ({methods.Length}):");
      foreach (var method in methods)
      {
        if (method.IsSpecialName) continue; // Skip property getters/setters and other special methods

        var accessibility = GetTypeAccessibility(method);
        var modifiers = new List<string>();
        if (method.IsStatic) modifiers.Add("static");
        if (method.IsAbstract) modifiers.Add("abstract");
        if (method.IsVirtual && !method.IsAbstract) modifiers.Add("virtual");
        // if (method.IsSealed) modifiers.Add("sealed");

        var modifierStr = modifiers.Count > 0 ? string.Join(" ", modifiers) + " " : "";
        var parameters = method.GetParameters();
        var paramStr = string.Join(", ", Array.ConvertAll(parameters, p => $"{p.ParameterType.Name} {p.Name}"));

        sb.AppendLine($"  {accessibility.ToString().ToLower()} {modifierStr}{method.ReturnType.Name} {method.Name}({paramStr})");
      }
      sb.AppendLine();

      // Constructors
      var constructors = type.GetConstructors(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static);
      sb.AppendLine($"Constructors ({constructors.Length}):");
      foreach (var ctor in constructors)
      {
        var accessibility = ctor.IsPublic ? "public" :
                           ctor.IsPrivate ? "private" :
                           ctor.IsFamily ? "protected" :
                           ctor.IsAssembly ? "internal" : "unknown";

        var modifiers = new List<string>();
        if (ctor.IsStatic) modifiers.Add("static");

        var modifierStr = modifiers.Count > 0 ? string.Join(" ", modifiers) + " " : "";
        var parameters = ctor.GetParameters();
        var paramStr = string.Join(", ", Array.ConvertAll(parameters, p => $"{p.ParameterType.Name} {p.Name}"));

        sb.AppendLine($"  {accessibility} {modifierStr}{type.Name}({paramStr})");
      }
      sb.AppendLine();

      // Custom attributes
      var attributes = type.GetCustomAttributes(false);
      sb.AppendLine($"Attributes ({attributes.Length}):");
      foreach (var attr in attributes)
      {
        sb.AppendLine($"  - {attr.GetType().Name}");
      }

      return sb.ToString();
    }

  }
}