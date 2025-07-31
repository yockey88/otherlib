using System;
using System.Runtime.InteropServices;

namespace OtherCsBindings
{
  [NativeClass("other::dotnet_object", true)]
  public class NativeObject
  {
    internal static unsafe delegate*<UInt64> GetNativeId;
    internal static unsafe delegate*<UInt64, void> SetNativeId;
    internal static unsafe delegate*<UInt64, IntPtr> GetNativeHandle;

    protected UInt64 NativeId
    {
      get { unsafe { return GetNativeId(); } }
      set { unsafe { SetNativeId(value); } }
    }
    protected IntPtr NativeHandle
    {
      get { unsafe { return GetNativeHandle(NativeId); } }
    }

    public NativeObject()
    {
    }

    ~NativeObject()
    {
    }
  }
}