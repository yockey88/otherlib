using System;
using OtherCsBindings;

namespace Other.Networking
{
  public static class Network
  {
    [NativeFunction("NetworkIsConnected")]
    internal static unsafe delegate*<NativeBool32> NativeIsConnected;
    [NativeFunction("NetworkGetRole")]
    internal static unsafe delegate*<int> NativeGetRole;

    public static bool IsConnected
    {
      get { unsafe { return NativeIsConnected(); } }
    }

    public static NetworkRole Role
    {
      get { unsafe { return (NetworkRole)NativeGetRole(); } }
    }
  }

  public enum NetworkRole
  {
    Client = 0,
    Server = 1,
  }
}