using System;
using OtherCsBindings;

namespace Other.Core
{
  public static class Filesystem
  {
    [NativeFunction("GetProgramFilesFolder")]
    internal static unsafe delegate*<NativeString, NativeString> NativeGetProgramFilesFolder;

    [NativeFunction("GetAppDataFolder")]
    internal static unsafe delegate*<NativeString, NativeBool32, NativeString> NativeGetAppDataFolder;

    public static string GetProgramFilesFolder(string app_name)
    {
      NativeString app_name_str = app_name;
      NativeString native_path;
      unsafe
      {
        native_path = NativeGetProgramFilesFolder(app_name_str);
      }
      return native_path.ToString();
    }

    public static string GetAppDataFolder(string app_name, bool create = false)
    {
      NativeString app_name_str = app_name;
      NativeString native_path;
      unsafe
      {
        native_path = NativeGetAppDataFolder(app_name_str, create);
      }
      return native_path.ToString();
    }
  }
}