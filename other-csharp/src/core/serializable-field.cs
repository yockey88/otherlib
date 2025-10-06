using System;

namespace Other.Core
{

  [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
  public class SerializableFieldAttribute : Attribute
  {
  }

}