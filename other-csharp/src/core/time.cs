using System;
using OtherCsBindings;

namespace Other.Core
{
  /// <summary>
  /// Provides access to engine timing information: delta time, elapsed time, and frame count.
  /// </summary>
  public static class Time
  {
    [NativeFunction("TimeGetDeltaTime")]
    internal static unsafe delegate*<float> NativeGetDeltaTime;
    [NativeFunction("TimeGetElapsedTime")]
    internal static unsafe delegate*<float> NativeGetElapsedTime;
    [NativeFunction("TimeGetFrameCount")]
    internal static unsafe delegate*<Int64> NativeGetFrameCount;

    /// <summary>Time in seconds since the last frame.</summary>
    public static float DeltaTime { get { unsafe { return NativeGetDeltaTime(); } } }

    /// <summary>Total elapsed time in seconds since the application started.</summary>
    public static float ElapsedTime { get { unsafe { return NativeGetElapsedTime(); } } }

    /// <summary>Total number of frames rendered since the application started.</summary>
    public static long FrameCount { get { unsafe { return NativeGetFrameCount(); } } }

    /// <summary>Frames per second (computed from delta time).</summary>
    public static float FPS
    {
      get
      {
        float dt = DeltaTime;
        return dt > 0 ? 1.0f / dt : 0;
      }
    }
  }
}