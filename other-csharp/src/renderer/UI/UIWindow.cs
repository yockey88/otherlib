using OtherCsBindings;

namespace Other
{
  public static class UI
  {
    [NativeFunction("BeginWindow")]
    internal static unsafe delegate*<NativeString, int, NativeBool32> NativeBeginWindow;

    [NativeFunction("EndWindow")]
    internal static unsafe delegate*<void> NativeEndWindow;

    [NativeFunction("BeginChild")]
    internal static unsafe delegate*<NativeString, int, NativeBool32> NativeBeginChild;
    [NativeFunction("EndChild")]
    internal static unsafe delegate*<void> NativeEndChild;


    public static bool BeginWindow(string title, int flags)
    {
      NativeString title_native = title;
      bool open = false;
      unsafe
      {
        NativeBool32 result = NativeBeginWindow(title_native, flags);
        open = result;
      }
      return open;
    }

    public static void EndWindow()
    {
      unsafe
      {
        NativeEndWindow();
      }
    }

    public static bool BeginChild(string name, int flags)
    {
      NativeString name_native = name;
      bool open = false;
      unsafe
      {
        NativeBool32 result = NativeBeginChild(name_native, flags);
        open = result;
      }
      return open;
    }

    public static void EndChild()
    {
      unsafe
      {
        NativeEndChild();
      }
    }
  }
}