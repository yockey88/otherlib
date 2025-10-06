using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;


namespace OtherCsBindings
{
  public class Logger
  {
    public enum LogLevel
    {
      Trace = 0,
      Debug = 1,
      Info = 2,
      Warning = 3,
      Error = 4,
      Critical = 5,
    }

    private static IntPtr native_handle;

    public static void Initialize(IntPtr native_handle)
    {
      Logger.native_handle = native_handle;
    }

    public static void LogTrace(string message, [CallerMemberName] string memberName = "", [CallerLineNumber] int lineNumber = 0)
    {
      LogMessage(message, LogLevel.Trace, memberName, lineNumber);
    }

    public static void LogDebug(string message, [CallerMemberName] string memberName = "", [CallerLineNumber] int lineNumber = 0)
    {
      LogMessage(message, LogLevel.Debug, memberName, lineNumber);
    }

    public static void LogInfo(string message, [CallerMemberName] string memberName = "", [CallerLineNumber] int lineNumber = 0)
    {
      LogMessage(message, LogLevel.Info, memberName, lineNumber);
    }

    public static void LogWarning(string message, [CallerMemberName] string memberName = "", [CallerLineNumber] int lineNumber = 0)
    {
      LogMessage(message, LogLevel.Warning, memberName, lineNumber);
    }

    public static void LogError(string message, [CallerMemberName] string memberName = "", [CallerLineNumber] int lineNumber = 0)
    {
      LogMessage(message, LogLevel.Error, memberName, lineNumber);
    }

    public static void LogCritical(string message, [CallerMemberName] string memberName = "", [CallerLineNumber] int lineNumber = 0)
    {
      LogMessage(message, LogLevel.Critical, memberName, lineNumber);
    }

    public static void Log(string message, LogLevel level, [CallerMemberName] string memberName = "", [CallerLineNumber] int lineNumber = 0)
    {
      LogMessage(message, level, memberName, lineNumber);
    }

    private static void LogMessage(string message, LogLevel level, [CallerMemberName] string memberName = "", [CallerLineNumber] int lineNumber = 0)
    {
      var message_string = $" [C#] {message} | {memberName} | {lineNumber}";
      if (native_handle == IntPtr.Zero)
      {
        Console.WriteLine($"Logger not initialized, cannot log message: {message_string}");
        return;
      }
      NativeString message_native = message_string;
      unsafe
      {
        NativeLogMessage(native_handle, message_native, (Int32)level);
      }
    }

    [NativeFunction("LogMessage", false)] // this is manually bound by host before entry
    internal static unsafe delegate*<IntPtr, NativeString, Int32, void> NativeLogMessage;


    // private NativeFunction
  }
}