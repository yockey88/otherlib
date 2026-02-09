using System;

namespace Other
{

  [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
  public class SerializableFieldAttribute : Attribute
  {
  }

}