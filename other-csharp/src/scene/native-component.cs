using System;

namespace Other
{
  [AttributeUsage(AttributeTargets.Class, Inherited = false)]
  public class NativeComponentAttribute : Attribute
  {
    public string Name { get; }
    public UInt64 Id => Fnv.Hash(Name);

    public NativeComponentAttribute(string name)
    {
      Name = name;
    }
  }
}