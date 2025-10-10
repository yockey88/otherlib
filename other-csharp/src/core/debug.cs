using System;
using System.Runtime.CompilerServices;

namespace Other.Core
{
  public class Debug
  {
    public static void Log(string message, [CallerMemberName] string memberName = "", [CallerLineNumber] int lineNumber = 0)
    {
      OtherCsBindings.Logger.LogInfo(message, memberName, lineNumber);
    }

    public static void Log(string message, OtherCsBindings.Logger.LogLevel level, [CallerMemberName] string memberName = "", [CallerLineNumber] int lineNumber = 0)
    {
      OtherCsBindings.Logger.Log(message, level, memberName, lineNumber);
    }

    public static void LogWarning(string message, [CallerMemberName] string memberName = "", [CallerLineNumber] int lineNumber = 0)
    {
      OtherCsBindings.Logger.LogWarning(message, memberName, lineNumber);
    }

    public static void LogError(string message, [CallerMemberName] string memberName = "", [CallerLineNumber] int lineNumber = 0)
    {
      OtherCsBindings.Logger.LogError(message, memberName, lineNumber);
    }
  }
}