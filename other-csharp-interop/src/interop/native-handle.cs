using System;

namespace OtherCsBindings
{
  public class NativeHandle : IEquatable<NativeHandle>
  {
    IntPtr native_ptr;

    public NativeHandle(IntPtr ptr)
    {
      native_ptr = ptr;
    }

    public IntPtr ToIntPtr() => native_ptr;

    public bool Equals(IntPtr ptr) => native_ptr == ptr;
    public bool Equals(NativeHandle other) => Equals(other.native_ptr);

    public override bool Equals(object obj) => obj is NativeHandle other && Equals(other);
    public override int GetHashCode() => native_ptr.GetHashCode();  

    public static bool operator ==(NativeHandle left, NativeHandle right) => left.Equals(right);
    public static bool operator !=(NativeHandle left, NativeHandle right) => !left.Equals(right);
  }
}