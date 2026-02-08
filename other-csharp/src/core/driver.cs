using System;
using OtherCsBindings;

namespace Other.Core
{
  /// <summary>
  /// Provides access to the application/driver state, configuration, and lifecycle.
  /// </summary>
  public static class Application
  {
    [NativeFunction("DriverGetState")]
    internal static unsafe delegate*<int> NativeGetState;
    [NativeFunction("DriverRequestShutdown")]
    internal static unsafe delegate*<void> NativeRequestShutdown;
    [NativeFunction("DriverGetProjectName")]
    internal static unsafe delegate*<NativeString> NativeGetProjectName;

    [NativeFunction("ConfigGetString")]
    internal static unsafe delegate*<NativeString, NativeString, NativeString, NativeString> NativeConfigGetString;
    [NativeFunction("ConfigGetInt")]
    internal static unsafe delegate*<NativeString, NativeString, int, int> NativeConfigGetInt;
    [NativeFunction("ConfigGetFloat")]
    internal static unsafe delegate*<NativeString, NativeString, float, float> NativeConfigGetFloat;
    [NativeFunction("ConfigGetBool")]
    internal static unsafe delegate*<NativeString, NativeString, NativeBool32, NativeBool32> NativeConfigGetBool;

    /// <summary>
    /// Current driver state as an integer matching the native driver_state enum.
    /// 0 = STOPPED, 1 = INITIALIZING, 2 = RUNNING, 3 = PAUSED, 4 = SHUTTING_DOWN
    /// </summary>
    public static DriverState State
    {
      get { unsafe { return (DriverState)NativeGetState(); } }
    }
    public static void RequestShutdown()
    {
      unsafe { NativeRequestShutdown(); }
    }

    public static string ProjectName
    {
      get
      {
        unsafe
        {
          NativeString result = NativeGetProjectName();
          return result.ToString();
        }
      }
    }

    public static string GetConfigString(string section, string key, string default_value = "")
    {
      unsafe
      {
        NativeString result = NativeConfigGetString(section, key, default_value);
        return result.ToString();
      }
    }

    public static int GetConfigInt(string section, string key, int default_value = 0)
    {
      unsafe { return NativeConfigGetInt(section, key, default_value); }
    }

    public static float GetConfigFloat(string section, string key, float default_value = 0)
    {
      unsafe { return NativeConfigGetFloat(section, key, default_value); }
    }

    public static bool GetConfigBool(string section, string key, bool default_value = false)
    {
      unsafe { return NativeConfigGetBool(section, key, default_value); }
    }
  }

  public enum DriverState
  {
    Stopped = 0,
    Initializing = 1,
    Running = 2,
    Paused = 3,
    ShuttingDown = 4,
  }
}