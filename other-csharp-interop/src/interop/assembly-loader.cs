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
    private static readonly Dictionary<Int32, List<Int32>> context_members = new();
    private static readonly Dictionary<Int32, Assembly> assemblies = new();
    /// each engine-loaded assembly gets its own collectible AssemblyLoadContext: .NET can only
    ///  unload whole contexts, never single assemblies, so per-assembly unload (script hot
    ///  reload) requires per-assembly contexts
    private static readonly Dictionary<Int32, AssemblyLoadContext> assembly_alcs = new();
    /// GCHandles are tracked as their IntPtr representation so entries can be removed when the
    ///  native side frees a handle — GCHandle struct copies go stale after Free and must never
    ///  be double-freed
    private static readonly Dictionary<Int32, HashSet<IntPtr>> handles = new();
    private static readonly Dictionary<IntPtr, Int32> handle_owners = new();

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
          if (IsResolvable(context, asm))
          {
            return asm;
          }
          return null;
        }

        foreach (var alc in AllLoadContexts())
        {
          foreach (var assembly in alc.Assemblies)
          {
            if (assembly.GetName().Name != asm_name.Name)
            {
              continue;
            }

            if (!IsResolvable(context, assembly))
            {
              return null;
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

    /// the runtime forbids non-collectible assemblies from referencing collectible ones, and
    ///  this handler is also attached to the (non-collectible) default context: when a
    ///  collectible context resolves a reference, the binder consults the default context
    ///  FIRST — handing it a collectible assembly poisons the whole bind. Returning null lets
    ///  resolution fall through to the requesting assembly's own context, where a
    ///  collectible→collectible reference is legal. Reflection lookups pass context == null
    ///  and create no reference edge, so they may see every assembly.
    private static bool IsResolvable(AssemblyLoadContext? requesting_context, Assembly asm)
    {
      if (requesting_context == null || requesting_context.IsCollectible)
      {
        return true;
      }
      return AssemblyLoadContext.GetLoadContext(asm)?.IsCollectible != true;
    }

    private static IEnumerable<AssemblyLoadContext> AllLoadContexts()
    {
      foreach (var alc in contexts.Values)
      {
        yield return alc;
      }
      foreach (var alc in assembly_alcs.Values)
      {
        yield return alc;
      }
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

      /// the context created here is a logical group: member assemblies are loaded into their
      ///  own per-assembly contexts (see LoadManagedAssembly) so they can be unloaded one at a
      ///  time — this context itself never contains engine assemblies
      var alc = new AssemblyLoadContext(name, true);
      alc.Resolving += ResolveAssembly;

      Int32 ctx_id = name.GetHashCode();
      contexts.Add(ctx_id, alc);
      context_members.Add(ctx_id, new List<Int32>());
      return ctx_id;
    }


    [UnmanagedCallersOnly]
    private static void UnloadAssemblyLoadContext(Int32 context_id)
    {
      try
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

        // unload every member assembly (each owns its own context)
        if (context_members.Remove(context_id, out var members))
        {
          foreach (var asm_id in members)
          {
            UnloadAssemblyById(asm_id);
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
      catch (Exception e)
      {
        Host.HandleException(e);
      }
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

        if (!contexts.TryGetValue(context_id, out var group_alc))
        {
          last_load_status = AssemblyLoadStatus.InvalidAssembly;
          Logger.LogError($"Failed to load assembly '{file_path}', couldn't find Load Context with id '{context_id}'");
          return -1;
        }

        if (group_alc == null)
        {
          last_load_status = AssemblyLoadStatus.CorruptContext;
          Logger.LogError($"Failed to load assembly '{file_path}', Load Context with id '{context_id}' is null");
          return -1;
        }

        /// load into a dedicated collectible context so this assembly can be unloaded (and hot
        ///  reloaded) individually — assemblies resolve each other through the Resolving
        ///  handler, which searches across every context
        var alc = new AssemblyLoadContext($"{group_alc.Name}:{Path.GetFileNameWithoutExtension(path)}", true);
        alc.Resolving += ResolveAssembly;

        Assembly? asm = null;

        using (var file = MemoryMappedFile.CreateFromFile(file_path!))
        {
          using var stream = file.CreateViewStream();
          asm = alc.LoadFromStream(stream);
        }

        var name = asm.GetName();

        Int32 asm_id = name.Name!.GetHashCode();
        if (assemblies.ContainsKey(asm_id))
        {
          last_load_status = AssemblyLoadStatus.Failed;
          Logger.LogError($"Failed to load assembly '{file_path}': an assembly named '{name.Name}' is already loaded, unload it first");
          alc.Unload();
          return -1;
        }

        Logger.LogDebug($"Successfully loaded assembly : '{name}' [{context_id}]");

        assemblies.Add(asm_id, asm);
        assembly_alcs.Add(asm_id, alc);
        if (context_members.TryGetValue(context_id, out var members))
        {
          members.Add(asm_id);
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
      try
      {
        /// assemblies cached from the default context (the bindings assembly and friends)
        ///  have no dedicated context and can never be unloaded
        if (!assembly_alcs.ContainsKey(asm_id))
        {
          Logger.LogError($"Couldn't unload assembly '{asm_id}', assembly not found!");
          return;
        }

        UnloadAssemblyById(asm_id);
      }
      catch (Exception e)
      {
        Host.HandleException(e);
      }
    }

    /// unloads one engine-loaded assembly for real: releases its GCHandles and cached
    ///  reflection objects (both would otherwise root the assembly forever), then unloads its
    ///  dedicated context. A subsequent LoadManagedAssembly yields a fresh instance — this is
    ///  the unload half of script hot reload.
    private static void UnloadAssemblyById(Int32 asm_id)
    {
      if (!assemblies.Remove(asm_id, out var asm))
      {
        return;
      }

      FreeAssemblyHandles(asm_id);
      TypeInterface.EvictAssemblyFromCaches(asm);

      if (assembly_alcs.Remove(asm_id, out var alc))
      {
        foreach (var members in context_members.Values)
        {
          members.Remove(asm_id);
        }
        alc.Unload();
      }
    }

    private static void FreeAssemblyHandles(Int32 asm_id)
    {
      if (!handles.Remove(asm_id, out var asm_handles))
      {
        return;
      }

      foreach (var ptr in asm_handles)
      {
        handle_owners.Remove(ptr);
        var handle = GCHandle.FromIntPtr(ptr);
        if (handle.IsAllocated)
        {
          handle.Free();
        }
      }
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
        hs = new HashSet<IntPtr>();
        handles.Add(asm_id, hs);
      }

      IntPtr ptr = GCHandle.ToIntPtr(handle);
      hs.Add(ptr);
      handle_owners[ptr] = asm_id;
    }

    /// must be called wherever a registered handle is freed, otherwise the stale entry would
    ///  be freed a second time on assembly unload — after the handle slot has been recycled,
    ///  that would free an unrelated live handle
    internal static void UnregisterHandle(IntPtr ptr)
    {
      if (!handle_owners.Remove(ptr, out var asm_id))
      {
        return;
      }

      if (handles.TryGetValue(asm_id, out var hs))
      {
        hs.Remove(ptr);
      }
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