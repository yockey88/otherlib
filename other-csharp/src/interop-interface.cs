using System;
using System.Collections.Generic;
using System.Collections.Immutable;
using System.Reflection;
using System.Text;
using System.Runtime.InteropServices;

#nullable enable
namespace OtherCsBindings
{
  public static class InteropInterface
  {

    internal readonly static Set<Type> cached_types = new();
    internal readonly static Set<MethodInfo> cached_methods = new();
    internal readonly static Set<FieldInfo> cached_fields = new();
    internal readonly static Set<PropertyInfo> cached_properties = new();
    internal readonly static Set<Attribute> cached_attributes = new();

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
        return null;
      }

      return type;
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

				if (out_types != null) {
					for (Int32 i = 0; i < types.Length; i++) {
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
		private static unsafe NativeString GetFullTypeName(Int32 type_id) {
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
		private static unsafe void GetTypeMethods(Int32 type, Int32* method_arr, Int32* count) {
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
				// LogMessage($" > Found {methods.Length} methods for type {t.FullName}", MessageLevel.Trace);

				*count = methods.Length;
				if (method_arr == null)
        {
					return;
				}

				for (Int32 i = 0; i < methods.Length; i++)
        {
					// LogMessage($"  > Adding method [class : {t.Name}] {methods[i].Name} to cache", MessageLevel.Trace);
					method_arr[i] = cached_methods.Add(methods[i]);
				}

				// LogMessage($"  > Added {methods.Length} methods to cache", MessageLevel.Trace);
			}
      catch (Exception ex)
      {
        Host.HandleException(ex);
      }
		}

		[UnmanagedCallersOnly]
		private static unsafe void GetTypeFields(Int32 type, Int32* field_arr, Int32* field_count) {
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

				ReadOnlySpan<FieldInfo> fields = t!.GetFields(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static);
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
					field_arr[i] = cached_fields.Add(fields[i]);
				}
			}
      catch (Exception ex)
      {
        Host.HandleException(ex);
      }
		}

		[UnmanagedCallersOnly]
		private static unsafe void GetTypeProperties(Int32 type, Int32* arr, Int32* count) {
			try
      {
				if (!cached_types.TryGet(type, out var t))
        {
					return;
				}

				ReadOnlySpan<PropertyInfo> properties = t!.GetProperties(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static);
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
					arr[i] = cached_properties.Add(properties[i]);
				}
			}
      catch (Exception ex)
      {
        Host.HandleException(ex);
      }
		}

		[UnmanagedCallersOnly]
		private static unsafe NativeBool32 HasAttribute(Int32 type , Int32 attr_type) 
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
					*count = 0;
					return;
				}

				*count = attrs.Length;

				if (attributes == null)
        {
					return;
				}

				for (Int32 i = 0; i < attrs.Length; i++)
        {
					Attribute attr = (Attribute)attrs[i];
					attributes[i] = cached_attributes.Add(attr);
				}
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