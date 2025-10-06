using System;
using System.Collections.Generic;
using System.IO;
using System.IO.MemoryMappedFiles;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.Loader;

#nullable enable
namespace OtherCsBindings
{

  [InteropBinding("AssemblyLoader")]
  internal static class AssemblyLoader
  {
    private enum CoreAssembly
    {
      SystemPrivateCoreLib,
      SystemRuntime,
      SystemConsole,
      SystemLinq,
      SystemCollections,
      SystemNetHttp,
      SystemIO,
      SystemThreading,
      NumCoreAssemblies
    }

    public enum AssemblyLoadStatus
    {
      Success,
      NotFound,
      Failed,
      InvalidPath,
      InvalidAssembly,
      CorruptContext,
      UnknownError
    }


    private static string[] core_assembly_names = new string[] {
      "System.Private.CoreLib",
      "System.Runtime",
      "System.Console",
      "System.Linq",
      "System.Collections",
      "System.Net.Http",
      "System.IO",
      "System.Threading"
    };


    private static Assembly[] core_assemblies = new Assembly[(int)CoreAssembly.NumCoreAssemblies];

    private static List<Type> core_types = new List<Type>();
    public static ReadOnlySpan<Type> CoreTypes => core_types.ToArray();

    private static AssemblyLoadStatus last_load_status = AssemblyLoadStatus.Success;

    private static readonly Dictionary<Type, AssemblyLoadStatus> load_errors = new();
    private static readonly Dictionary<Int32, AssemblyLoadContext> contexts = new();
    private static readonly Dictionary<Int32, Assembly> assemblies = new();
    private static Dictionary<Int32, List<GCHandle>> handles = new();

    static Assembly? current_executing_assembly = null;
    static AssemblyLoadContext? current_load_context = null;

    private static bool core_assemblies_loaded = false;
    public static bool CoreAssembliesLoaded
    {
      get => core_assemblies_loaded;
    }

    static AssemblyLoader()
    {
      load_errors.Add(typeof(BadImageFormatException), AssemblyLoadStatus.InvalidAssembly);
      load_errors.Add(typeof(FileNotFoundException), AssemblyLoadStatus.NotFound);
      load_errors.Add(typeof(FileLoadException), AssemblyLoadStatus.Failed);
      load_errors.Add(typeof(ArgumentNullException), AssemblyLoadStatus.InvalidPath);
      load_errors.Add(typeof(ArgumentException), AssemblyLoadStatus.InvalidAssembly);
      load_errors.Add(typeof(NullReferenceException), AssemblyLoadStatus.CorruptContext);

      current_load_context = AssemblyLoadContext.GetLoadContext(typeof(AssemblyLoader).Assembly);
      if (current_load_context == null)
      {
        return;
      }

      current_load_context!.Resolving += ResolveAssembly;
      // current_load_context!.Unloading += ctx => {
      //   foreach (var asm in ctx.Assemblies)
      //   {
      //     var asm_name = asm.GetName();
      //     Int32 asm_id = asm_name.Name!.GetHashCode();
      //     assemblies.Remove(asm_id);
      //   }
      // };
      CacheAssemblies();
    }

    private static void CacheAssemblies()
    {
      foreach (var assembly in current_load_context!.Assemblies)
      {
        int assemblyId = assembly.GetName().Name!.GetHashCode();
        assemblies.Add(assemblyId, assembly);
      }
    }

    internal static Type? GetNetCoreType(NativeString name)
    {
      Type? type = null;

      for (int i = 0; i < (int)CoreAssembly.NumCoreAssemblies; i++)
      {
        if (core_assemblies[i] == null)
        {
          continue;
        }

        type = core_assemblies[i].GetType(name!);
        if (type != null)
        {
          return type;
        }
      }

      return type;
    }

    internal static Assembly? ResolveAssembly(AssemblyLoadContext? context, AssemblyName asm_name)
    {
      try
      {
        Int32 asm_id = asm_name.Name!.GetHashCode();
        if (assemblies.TryGetValue(asm_id, out var asm))
        {
          return asm;
        }

        foreach (var alc in contexts.Values)
        {
          foreach (var assembly in alc.Assemblies)
          {
            if (assembly.GetName().Name != asm_name.Name)
            {
              continue;
            }

            assemblies.Add(asm_id, assembly);
            return assembly;
          }
        }
      }
      catch (Exception e)
      {
        Host.HandleException(e);
      }
      return null;
    }

    internal static List<Assembly> GetFullLoadedAssemblyContext()
    {
      List<Assembly> loaded_assemblies = new List<Assembly>();
      foreach (var asm in assemblies.Values)
      {
        loaded_assemblies.Add(asm);
      }
      return loaded_assemblies;
    }

    internal static bool TryGetAssembly(Int32 id, out Assembly? asm)
    {
      return assemblies.TryGetValue(id, out asm);
    }

    [UnmanagedCallersOnly]
    private static Int32 CreateAssemblyLoadContext(NativeString context_name)
    {
      string? name = context_name;
      if (name == null)
      {
        Console.WriteLine("Failed to create AssemblyLoadContext, name is null.");
        return -1;
      }

      var alc = new AssemblyLoadContext(name, true);

      alc.Resolving += ResolveAssembly;
      alc.Unloading += ctx =>
      {
        foreach (var asm in ctx.Assemblies)
        {
          var asm_name = asm.GetName();
          Int32 asm_id = asm_name.Name!.GetHashCode();
          assemblies.Remove(asm_id);
        }
      };

      Int32 ctx_id = name.GetHashCode();
      contexts.Add(ctx_id, alc);
      return ctx_id;
    }


    [UnmanagedCallersOnly]
    private static void UnloadAssemblyLoadContext(Int32 context_id)
    {
      if (!contexts.TryGetValue(context_id, out var alc))
      {
        Logger.LogError($"Cannot unload AssemblyLoadContext '{context_id}', it was either never loaded or already unloaded.");
        return;
      }

      if (alc == null)
      {
        Logger.LogError($"Cannot unload AssemblyLoadContext '{context_id}', it is null.");
        return;
      }

      foreach (var assembly in alc.Assemblies)
      {
        var asm_name = assembly.GetName();
        int asm_id = asm_name.Name!.GetHashCode();

        if (!handles.TryGetValue(asm_id, out var hs))
        {
          continue;
        }

        foreach (var h in hs)
        {
          if (!h.IsAllocated || h.Target == null)
          {
            continue;
          }
          h.Free();
        }
      }

      TypeInterface.cached_types.Clear();
      TypeInterface.cached_methods.Clear();
      TypeInterface.cached_fields.Clear();
      TypeInterface.cached_properties.Clear();
      TypeInterface.cached_attributes.Clear();

      contexts.Remove(context_id);
      alc.Unload();
    }

    [UnmanagedCallersOnly]
    private static Int32 LoadManagedAssembly(Int32 context_id, NativeString file_path)
    {
      try
      {
        string path = file_path.ToString()!;
        Logger.LogDebug($"Loading assembly '{path}' [{context_id}]");

        last_load_status = AssemblyLoadStatus.Success;
        if (string.IsNullOrEmpty(path))
        {
          last_load_status = AssemblyLoadStatus.InvalidPath;
          Logger.LogError($"Failed to load assembly : '{file_path}', path is null or empty");
          return -1;
        }

        if (!File.Exists(path))
        {
          last_load_status = AssemblyLoadStatus.NotFound;
          Logger.LogError($"Failed to load assembly : '{file_path}', file does not exist");
          return -1;
        }

        if (!contexts.TryGetValue(context_id, out var alc))
        {
          last_load_status = AssemblyLoadStatus.InvalidAssembly;
          Logger.LogError($"Failed to load assembly '{file_path}', couldn't find Load Context with id '{context_id}'");
          return -1;
        }

        if (alc == null)
        {
          last_load_status = AssemblyLoadStatus.CorruptContext;
          Logger.LogError($"Failed to load assembly '{file_path}', Load Context with id '{context_id}' is null");
          return -1;
        }

        Assembly? asm = null;

        using (var file = MemoryMappedFile.CreateFromFile(file_path!))
        {
          using var stream = file.CreateViewStream();
          asm = alc.LoadFromStream(stream);
        }

        var name = asm.GetName();
        Logger.LogDebug($"Successfully loaded assembly : '{name}' [{context_id}]");

        Int32 asm_id = name.Name!.GetHashCode();
        try
        {
          assemblies.Add(asm_id, asm);
        }
        catch (Exception e)
        {
          last_load_status = AssemblyLoadStatus.Failed;
          Host.HandleException(e);
          return -1;
        }

        last_load_status = AssemblyLoadStatus.Success;
        return asm_id;
      }
      catch (Exception e)
      {
        Host.HandleException(e);
        return -1;
      }
    }

    [UnmanagedCallersOnly]
    private static void UnloadManagedAssembly(Int32 asm_id)
    {
      if (!assemblies.TryGetValue(asm_id, out var asm))
      {
        Logger.LogError($"Couldn't unload assembly '{asm_id}', assembly not found!");
        return;
      }

      assemblies.Remove(asm_id);
    }

    [UnmanagedCallersOnly]
    private static AssemblyLoadStatus GetLastLoadStatus() => last_load_status;

    [UnmanagedCallersOnly]
    private static NativeString GetAssemblyName(Int32 asm_id)
    {
      if (!assemblies.TryGetValue(asm_id, out var asm))
      {
        Logger.LogError($"Couldn't get assembly name for assembly '{asm_id}', assembly not found!");
        return "<unknown>";
      }

      var asm_name = asm.GetName();
      return asm_name.Name;
    }

    internal static void RegisterHandle(Assembly asm, GCHandle handle)
    {
      var asm_name = asm.GetName();
      Int32 asm_id = asm_name.Name!.GetHashCode();

      if (!handles.TryGetValue(asm_id, out var hs))
      {
        handles.Add(asm_id, new List<GCHandle>());
        hs = handles[asm_id];
      }

      hs.Add(handle);
    }

    public static void LoadCoreAssemblies()
    {
      try
      {
        for (int i = 0; i < (int)CoreAssembly.NumCoreAssemblies; i++)
        {
          Assembly? core_asm = LoadNetCoreAssembly((CoreAssembly)i, core_assembly_names[i]);
          if (core_asm == null)
          {
            Logger.LogError($"Failed to load core assembly '{core_assembly_names[i]}', assembly is null.");
            break;
          }

          core_assemblies[i] = core_asm;
        }

        if (current_executing_assembly == null)
        {
          current_executing_assembly = Assembly.GetExecutingAssembly();
          if (current_executing_assembly == null)
          {
            Logger.LogError("Failed to get current executing assembly, it is null.");
            return;
          }
          else
          {
            Logger.LogDebug($"Bindings Assembly Loaded : [{current_executing_assembly.FullName}]");
          }
        }

        core_assemblies_loaded = true;
      }
      catch (Exception e)
      {
        Host.HandleException(e);
      }
    }

    private static Assembly? LoadNetCoreAssembly(CoreAssembly assembly_type, string assembly_name)
    {
      try
      {
        Assembly? assembly = null;
        if (core_assemblies[(int)assembly_type] == null)
        {
          assembly = Assembly.Load(assembly_name);
          if (assembly == null)
          {
            Logger.LogError($"Failed to load core assembly '{assembly_name}', assembly is null");
            return null;
          }

          ReadOnlySpan<Type> types = assembly.GetTypes();
          core_assemblies[(int)assembly_type] = assembly;

          Logger.LogDebug($"Loaded core assembly : {core_assemblies[(int)assembly_type].FullName}");
          foreach (var type in types)
          {
            core_types.Add(type);
          }
        }
        else
        {
          assembly = core_assemblies[(int)assembly_type];
        }

        return assembly;
      }
      catch (Exception e)
      {
        Host.HandleException(e);
        return null;
      }
    }
  }

}
#nullable disable