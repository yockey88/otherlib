using System;

namespace OtherCsBindings
{
  public class NativeClassAttribute : Attribute, OtherAttribute
  {
    public string Name { get; private set; }
    public bool Required { get; private set; }
    public NativeClassAttribute(string name, bool required = false)
    {
      Name = name;
      Required = required;
    }
  }
}