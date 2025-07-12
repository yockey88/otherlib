using System;
using System.Runtime.InteropServices;

namespace Other
{

  [StructLayout(LayoutKind.Sequential)]
  public struct NString : IDisposable
  {
    internal IntPtr native_string;
    private NBool32 disposed;

    public void Dispose()
    {
      if (!disposed)
      {
        if (native_string != IntPtr.Zero)
        {
          Marshal.FreeCoTaskMem(native_string);
          native_string = IntPtr.Zero;
        }

        disposed = true;
      }

      GC.SuppressFinalize(this);
    }

    public override string? ToString() => this;

    public static NString Null() => new NString() { native_string = IntPtr.Zero };

    public static implicit operator NString(string? str) => new() { native_string = Marshal.StringToCoTaskMemAuto(str) };
    public static implicit operator string?(NString str) => Marshal.PtrToStringAuto(str.native_string);
  }

}