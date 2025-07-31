using System;
using System.Runtime.InteropServices;

namespace OtherCsBindings
{

  internal static class GarbageCollector
  {
    [UnmanagedCallersOnly]
    internal static void CollectGarbage(Int32 generation, GCCollectionMode mode, NativeBool32 blocking, NativeBool32 compacting)
    {
      try
      {
        if (generation < 0)
        {
          GC.Collect();
        }
        else
        {
          GC.Collect(generation, mode, blocking, compacting);
        }
      }
      catch (Exception e)
      {
        Host.HandleException(e);
      }
    }

    [UnmanagedCallersOnly]
    internal static void WaitForPendingFinalizers()
    {
      try
      {
        GC.WaitForPendingFinalizers();
      }
      catch (Exception e)
      {
        Host.HandleException(e);
      }
    }
  }

}