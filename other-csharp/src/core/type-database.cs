using System;
using System.Collections.Generic;

namespace Other
{
  internal static class TypeDatabase
  {
    struct NativeTypeData
    {
      public Type type;
    }
    private static Dictionary<ulong, NativeTypeData> componentTypes = new Dictionary<ulong, NativeTypeData>();

    public static void RegisterComponentType(string native_name)
    {
      ulong id = Fnv.Hash(native_name);
      componentTypes[id] = new NativeTypeData() {};
    }
  }
}