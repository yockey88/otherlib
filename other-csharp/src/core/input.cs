using System;
using System.Numerics;
using OtherCsBindings;

namespace Other
{
  public static class Input
  {
    [NativeFunction("InputIsKeyDown")]
    internal static unsafe delegate*<int, NativeBool32> NativeIsKeyDown;
    [NativeFunction("InputIsKeyPressed")]
    internal static unsafe delegate*<int, NativeBool32> NativeIsKeyPressed;
    [NativeFunction("InputIsMouseButtonDown")]
    internal static unsafe delegate*<int, NativeBool32> NativeIsMouseButtonDown;
    [NativeFunction("InputIsMouseButtonClicked")]
    internal static unsafe delegate*<int, NativeBool32> NativeIsMouseButtonClicked;
    [NativeFunction("InputGetMousePosition")]
    internal static unsafe delegate*<float*, float*, void> NativeGetMousePosition;
    [NativeFunction("InputGetMouseDelta")]
    internal static unsafe delegate*<float*, float*, void> NativeGetMouseDelta;
    [NativeFunction("InputGetMouseWheel")]
    internal static unsafe delegate*<float> NativeGetMouseWheel;

    public static bool IsKeyDown(KeyCode key) { unsafe { return NativeIsKeyDown((int)key); } }
    public static bool IsKeyPressed(KeyCode key) { unsafe { return NativeIsKeyPressed((int)key); } }

    public static bool IsMouseButtonDown(MouseButton button) { unsafe { return NativeIsMouseButtonDown((int)button); } }
    public static bool IsMouseButtonClicked(MouseButton button) { unsafe { return NativeIsMouseButtonClicked((int)button); } }

    public static Vec2 MousePosition
    {
      get
      {
        float x = 0, y = 0;
        unsafe { NativeGetMousePosition(&x, &y); }
        return new Vec2(x, y);
      }
    }

    public static Vec2 MouseDelta
    {
      get
      {
        float x = 0, y = 0;
        unsafe { NativeGetMouseDelta(&x, &y); }
        return new Vec2(x, y);
      }
    }

    public static float MouseWheel
    {
      get { unsafe { return NativeGetMouseWheel(); } }
    }
  }

  public enum MouseButton
  {
    Left = 0,
    Right = 1,
    Middle = 2,
    Extra1 = 3,
    Extra2 = 4,
  }

  public enum KeyCode
  {
    A = 4, B = 5, C = 6, D = 7, E = 8, F = 9, G = 10, H = 11,
    I = 12, J = 13, K = 14, L = 15, M = 16, N = 17, O = 18, P = 19,
    Q = 20, R = 21, S = 22, T = 23, U = 24, V = 25, W = 26, X = 27,
    Y = 28, Z = 29,

    Num1 = 30, Num2 = 31, Num3 = 32, Num4 = 33, Num5 = 34,
    Num6 = 35, Num7 = 36, Num8 = 37, Num9 = 38, Num0 = 39,

    Return = 40, Escape = 41, Backspace = 42, Tab = 43, Space = 44,

    F1 = 58, F2 = 59, F3 = 60, F4 = 61, F5 = 62, F6 = 63,
    F7 = 64, F8 = 65, F9 = 66, F10 = 67, F11 = 68, F12 = 69,

    Right = 79, Left = 80, Down = 81, Up = 82,

    LeftCtrl = 224, LeftShift = 225, LeftAlt = 226,
    RightCtrl = 228, RightShift = 229, RightAlt = 230,

    Delete = 76, Home = 74, End = 77, PageUp = 75, PageDown = 78,
    Insert = 73,
  }
}
