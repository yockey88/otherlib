using System;
using System.Runtime.InteropServices;

namespace OtherCsBindings
{
  [StructLayout(LayoutKind.Sequential, Pack = 1)]
  public struct NativeBool32 {
    public uint Value { 
      get; 
      set; 
    }

    public static implicit operator NativeBool32(bool val) => new() { Value = val ? 1u : 0u };
    public static implicit operator bool(NativeBool32 b) => b.Value > 0;
  }
  
  internal enum ManagedType {
		Unknown,

		SByte,
		Byte,
		Short,
		UShort,
		Int,
		UInt,
		Long,
		ULong,

		Float,
		Double,

		Bool,

		Pointer
	};

  [StructLayout(LayoutKind.Sequential, Pack = 1)]
  struct Argv
  {
    public IntPtr arena_native_handle;
    public IntPtr logger_native_handle;
    public IntPtr type_database_native_handle;
    public IntPtr scripting_environment_native_handle;
  }

  [InteropBinding("Host")]
  internal static class Host
  {
    public static void HandleException(Exception ex)
    {
      Logger.LogError($"Unhandled exception: {ex.Message}\n{ex.StackTrace}");
      if (ex.InnerException != null)
      {
        Logger.LogError($"Inner exception: {ex.InnerException.Message}\n{ex.InnerException.StackTrace}");
      }
    }

    [UnmanagedCallersOnly]
    private static unsafe void Entry(Argv args)
    {
      Logger.Initialize(args.logger_native_handle);
      Logger.LogInfo("Initializing Other Environment .NET Host");

      AssemblyLoader.LoadCoreAssemblies();

      NativeFunctionManager.DiscoverAndRegisterFunctions();
    }

  }
  
}