using System;

namespace Other
{
  public class NativeFieldAttribute : Attribute
  {
    public string Name { get; }
    public UInt64 Id => Fnv.Hash(Name);

    public NativeFieldAttribute(string name)
    {
      Name = name;
    }
  }
}