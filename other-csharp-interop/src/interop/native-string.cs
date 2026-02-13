using System;
using System.Runtime.InteropServices;

#nullable enable
namespace OtherCsBindings
{

  [StructLayout(LayoutKind.Sequential)]
  public struct NativeString : IDisposable
  {
    internal IntPtr native_string;
    private NativeBool32 disposed;

    public NativeString()
    {
      native_string = IntPtr.Zero;
      disposed = false;
    }

    public NativeString(string str)
    {
      native_string = Marshal.StringToCoTaskMemAuto(str);
      disposed = false;
    }

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

    public void Assign(string str)
    {
      if (native_string != IntPtr.Zero)
      {
        Marshal.FreeCoTaskMem(native_string);
      }

      native_string = Marshal.StringToCoTaskMemAuto(str);
      disposed = false;
    }

    public override string? ToString() => this;

    public static NativeString Null() => new NativeString() { native_string = IntPtr.Zero };

    public static implicit operator NativeString(string? str) => new NativeString(str!);
    public static implicit operator string?(NativeString str) => Marshal.PtrToStringAuto(str.native_string);
  }

}
#nullable disable