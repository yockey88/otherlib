using System;
using OtherCsBindings;

namespace Other.Networking
{
  public static class Steam
  {
    [NativeFunction("SteamIsAvailable")]
    internal static unsafe delegate*<NativeBool32> NativeIsAvailable;
    [NativeFunction("SteamPlayerId")]
    internal static unsafe delegate*<ulong> NativePlayerId;
    [NativeFunction("SteamPlayerName")]
    internal static unsafe delegate*<NativeString> NativePlayerName;
    [NativeFunction("SteamOpenInviteDialog")]
    internal static unsafe delegate*<void> NativeOpenInviteDialog;

    public static bool IsAvailable
    {
      get { unsafe { return NativeIsAvailable(); } }
    }

    public static ulong PlayerId
    {
      get { unsafe { return NativePlayerId(); } }
    }

    public static string PlayerName
    {
      get { unsafe { return NativePlayerName().ToString() ?? ""; } }
    }

    /// Opens the overlay invite dialog for the current lobby (steam sessions only).
    public static void OpenInviteDialog()
    {
      unsafe { NativeOpenInviteDialog(); }
    }
  }
}
