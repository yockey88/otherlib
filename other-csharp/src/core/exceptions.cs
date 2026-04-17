using System;

namespace Other
{
  public class MissingComponentException : Exception
  {
    public MissingComponentException(string message) : base(message)
    {
    }
  }
}