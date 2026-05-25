using System;
using OtherCsBindings;

namespace Other
{
  [AttributeUsage(AttributeTargets.Method, AllowMultiple = true)]
  public class CallbackBindingAttribute : Attribute
  {
    public NativeString BindingName;
    public CallbackBindingAttribute(string binding_name)
    {
      BindingName = new NativeString(binding_name);
    }
  }
}