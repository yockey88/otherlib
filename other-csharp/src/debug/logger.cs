using System;
using System.Runtime.CompilerServices;

namespace Other.Debug
{
  public class Console
  {
    public static void Log(string message, [CallerMemberName] string memberName = "", [CallerFilePath] string filePath = "", [CallerLineNumber] int lineNumber = 0)
    {
      OtherCsBindings.Logger.LogInfo(message, memberName, filePath, lineNumber);
    }

    public static void Log(string message, OtherCsBindings.Logger.LogLevel level, [CallerMemberName] string memberName = "", [CallerFilePath] string filePath = "", [CallerLineNumber] int lineNumber = 0)
    {
      OtherCsBindings.Logger.Log(message, level, memberName, filePath, lineNumber);
    }
  }
}