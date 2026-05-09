using System;
using System.Runtime.CompilerServices;
using Other.Core;

namespace Other
{
  public class Debug
  {
    public static void Log(string message, [CallerMemberName] string memberName = "", [CallerLineNumber] int lineNumber = 0)
    {
      Logger.Log(message, memberName, lineNumber);
    }

    public static void Warn(string message, [CallerMemberName] string memberName = "", [CallerLineNumber] int lineNumber = 0)
    {
      Logger.LogWarning(message, memberName, lineNumber);
    }

    public static void Error(string message, [CallerMemberName] string memberName = "", [CallerLineNumber] int lineNumber = 0)
    {
      Logger.LogError(message, memberName, lineNumber);
    }
  }
}