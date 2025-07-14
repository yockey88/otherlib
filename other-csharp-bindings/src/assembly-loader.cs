using System;
using System.Collections.Generic;
using System.Reflection;

namespace OtherCsBindings
{

  public class AssemblyLoader
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

    static Assembly? dotother_assembly = null;

    public static void LoadCoreAssemblies()
    {
      for (int i = 0; i < (int)CoreAssembly.NumCoreAssemblies; i++)
      {
        Assembly? core_asm = LoadNetCoreAssembly((CoreAssembly)i, core_assembly_names[i]);
        if (core_asm == null)
        {
          Console.WriteLine($"Failed to load core assembly '{core_assembly_names[i]}', assembly is null");
          break;
        }

        core_assemblies[i] = core_asm;
      }

      try
      {
        if (dotother_assembly == null)
        {
          dotother_assembly = Assembly.GetExecutingAssembly();
          if (dotother_assembly == null)
          {
            Console.WriteLine($"Failed to load system assembly, assembly is null");
            return;
          }

          ReadOnlySpan<Type> types = dotother_assembly.GetTypes();
          foreach (var type in types)
          {
            Console.WriteLine($"> System Type : {type.FullName}");
          }
        }
      }
      catch (Exception e)
      {
        // LogMessage($"Failed to load system system assembly | \n\t{e.StackTrace}", MessageLevel.Error);
      }
    }
    
    private static Assembly? LoadNetCoreAssembly(CoreAssembly assembly_type, string assembly_name) {
      try {
        Assembly? assembly = null;
        if (core_assemblies[(int)assembly_type] == null) {
          assembly = Assembly.Load(assembly_name);
          if (assembly == null) {
            Console.WriteLine($"Failed to load core assembly '{assembly_name}', assembly is null");
            // LogMessage($"Failed to load core assembly '{assembly_name}', assembly is null", MessageLevel.Error);
            return null;
          }

          ReadOnlySpan<Type> types = assembly.GetTypes();
          core_assemblies[(int)assembly_type] = assembly;

          // LogMessage($"Loaded core assembly : {core_assemblies[(int)assembly_type].FullName}", MessageLevel.Trace);
          // LogMessage($" > Number of types in core assembly : {types.Length}", MessageLevel.Trace);
          foreach (var type in types) {
            core_types.Add(type);
          }
        } else {
          // LogMessage($"Core assembly already loaded : {core_assemblies[(int)assembly_type].FullName}", MessageLevel.Trace);
          assembly = core_assemblies[(int)assembly_type];
        }

        return assembly;
      } catch (Exception e) {
        Console.WriteLine($"Failed to load core assembly '{assembly_name}' | \n\t{e.StackTrace}");
        // LogMessage($"Failed to load core assembly '{assembly_name}' | \n\t{e.StackTrace}", MessageLevel.Error);
        return null;
      }
    }
  }

}