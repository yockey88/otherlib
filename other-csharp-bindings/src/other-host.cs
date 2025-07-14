using System;
using System.Runtime.InteropServices;

namespace OtherCsBindings
{
  [StructLayout(LayoutKind.Sequential, Pack = 1)]
  public struct NBool32 {
    public uint Value { 
      get; 
      set; 
    }

    public static implicit operator NBool32(bool val) => new() { Value = val ? 1u : 0u };
    public static implicit operator bool(NBool32 b) => b.Value > 0;
  }

  [StructLayout(LayoutKind.Sequential, Pack = 1)]
  struct Argv
  {
  }


  internal static class Host
  {
    [UnmanagedCallersOnly]
    private static unsafe void Entry(Argv args)
    {
      AssemblyLoader.LoadCoreAssemblies();
    }
  }
  
}