using System;
using System.Runtime.InteropServices;

namespace OtherCsBindings
{
  [NativeClass("other::dotnet_object", true)]
  public class NativeObject
  {
    string class_name;

    Int64 script_object_id;
    IntPtr native_ptr;

    protected Int64 ScriptObjectId
    {
      get { return script_object_id; }
      set { script_object_id = value; }
    }

    protected IntPtr NativeHandle
    {
      get { return native_ptr; }
    }

    public NativeObject(Int64 script_object_id, IntPtr ptr, NativeString class_name)
    {
      this.class_name = class_name.ToString();
      this.script_object_id = script_object_id;
      this.native_ptr = ptr;
    }

    ~NativeObject()
    {
    }

    public bool Equals(nint ptr) => NativeHandle == ptr;
    public bool Equals(NativeObject other) => NativeHandle == other.NativeHandle;
    public override bool Equals(object obj) => obj is NativeObject other && Equals(other.NativeHandle);
    public override int GetHashCode() => NativeHandle.GetHashCode();
  }
}