using System;

namespace Other
{
  public class TypeBinder
  {
    public TypeBinder()
    {
    }

    public static void BindComponent(string native_name)
    {
      UInt64 component_id = Fnv.Hash(native_name);
    }
  }
}