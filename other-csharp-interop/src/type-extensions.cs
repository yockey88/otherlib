using System;
using System.Linq;

namespace OtherCsBindings
{
  public static class TypeExtensionMethods
  {
    public static T GetCustomAttribute<T>(this Type type, T attr) where T : Attribute
    {
      object[] attrs = type.GetCustomAttributes(false);
      return attrs.OfType<T>().FirstOrDefault();
    }
  }
}