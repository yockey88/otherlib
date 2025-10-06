using System;

namespace OtherCsBindings
{
  [AttributeUsage(AttributeTargets.Class, Inherited = false)]
  internal class InteropBindingAttribute : Attribute, OtherAttribute
  {
    public string Name { get; set; }

    public InteropBindingAttribute(string name)
    {
      Name = name;
    }
  }
}