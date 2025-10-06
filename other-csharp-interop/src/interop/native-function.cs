using System;

namespace OtherCsBindings
{
  [AttributeUsage(AttributeTargets.Field, Inherited = false)]
  public class NativeFunctionAttribute : Attribute, OtherAttribute
  {
    public string Name { get; set; }
    public bool AutoBind { get; set; }

    public NativeFunctionAttribute(string name, bool auto_bind = true)
    {
      Name = name;
      AutoBind = auto_bind;
    }
  }
}