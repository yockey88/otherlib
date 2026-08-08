using System;
using OtherCsBindings;

namespace Other.Networking
{
  public enum NetworkRole
  {
    None = 0,
    Host = 1,
    Client = 2,
  }

  public static class Network
  {
    [NativeFunction("NetworkIsConnected")]
    internal static unsafe delegate*<NativeBool32> NativeIsConnected;
    [NativeFunction("NetworkGetRole")]
    internal static unsafe delegate*<int> NativeGetRole;
    [NativeFunction("NetworkLocalPeerId")]
    internal static unsafe delegate*<int> NativeLocalPeerId;
    [NativeFunction("NetworkHost")]
    internal static unsafe delegate*<ushort, NativeBool32> NativeHost;
    [NativeFunction("NetworkJoin")]
    internal static unsafe delegate*<NativeString, ushort, NativeBool32> NativeJoin;
    [NativeFunction("NetworkLeave")]
    internal static unsafe delegate*<void> NativeLeave;
    [NativeFunction("NetworkSendEvent")]
    internal static unsafe delegate*<NativeString, byte*, int, void> NativeSendEvent;
    [NativeFunction("NetworkBroadcastEvent")]
    internal static unsafe delegate*<NativeString, byte*, int, void> NativeBroadcastEvent;
    [NativeFunction("NetworkCopyEventPayload")]
    internal static unsafe delegate*<byte*, int, int> NativeCopyEventPayload;

    public static bool IsConnected
    {
      get { unsafe { return NativeIsConnected(); } }
    }

    public static NetworkRole Role
    {
      get { unsafe { return (NetworkRole)NativeGetRole(); } }
    }

    public static bool IsHost => Role == NetworkRole.Host;

    public static ushort LocalPeerId
    {
      get { unsafe { return (ushort)NativeLocalPeerId(); } }
    }

    /// port 0 uses the networking.port config value
    public static bool Host(ushort port = 0)
    {
      unsafe { return NativeHost(port); }
    }

    public static bool Join(string address, ushort port = 0)
    {
      unsafe { return NativeJoin(address, port); }
    }

    public static void Leave()
    {
      unsafe { NativeLeave(); }
    }

    /// client -> host
    public static void SendEvent(string name, byte[]? payload = null)
    {
      unsafe
      {
        fixed (byte* data = payload)
        {
          NativeSendEvent(name, data, payload?.Length ?? 0);
        }
      }
    }

    /// host -> all
    public static void BroadcastEvent(string name, byte[]? payload = null)
    {
      unsafe
      {
        fixed (byte* data = payload)
        {
          NativeBroadcastEvent(name, data, payload?.Length ?? 0);
        }
      }
    }

    public static Action<ushort>? OnPeerJoined;
    public static Action<ushort>? OnPeerLeft;
    public static Action<ushort, string, byte[]>? OnEvent;

    // native entry points, invoked by name each call so assembly reloads stay safe
    internal static void DispatchPeerJoined(ushort peer) => OnPeerJoined?.Invoke(peer);
    internal static void DispatchPeerLeft(ushort peer) => OnPeerLeft?.Invoke(peer);

    internal static void DispatchEvent(ushort sender, string name, int payloadSize)
    {
      if (OnEvent == null)
      {
        return;
      }

      byte[] payload = payloadSize > 0 ? new byte[payloadSize] : Array.Empty<byte>();
      if (payloadSize > 0)
      {
        unsafe
        {
          fixed (byte* data = payload)
          {
            NativeCopyEventPayload(data, payload.Length);
          }
        }
      }
      OnEvent.Invoke(sender, name, payload);
    }
  }
}
