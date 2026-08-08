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
    [NativeFunction("NetworkHostSteam")]
    internal static unsafe delegate*<NativeBool32> NativeHostSteam;
    [NativeFunction("NetworkJoinLobby")]
    internal static unsafe delegate*<ulong, NativeBool32> NativeJoinLobby;
    [NativeFunction("NetworkSpawn")]
    internal static unsafe delegate*<ulong, ushort, NativeBool32> NativeSpawn;
    [NativeFunction("NetworkSyncComponent")]
    internal static unsafe delegate*<ulong, NativeString, NativeBool32> NativeSyncComponent;
    [NativeFunction("NetworkIsMine")]
    internal static unsafe delegate*<ulong, NativeBool32> NativeIsMine;
    [NativeFunction("NetworkRequestOp")]
    internal static unsafe delegate*<NativeString, ulong, byte*, int, NativeBool32> NativeRequestOp;

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

    /// Hosts over steam (lobby + P2P listen); Host() keeps honoring networking.transport.
    public static bool HostSteam()
    {
      unsafe { return NativeHostSteam(); }
    }

    public static bool JoinLobby(ulong lobbyId)
    {
      unsafe { return NativeJoinLobby(lobbyId); }
    }

    /// Host-only: register an object for replication; the SPAWN reaches every
    /// client on the next snapshot pass.
    public static bool Spawn(SceneObject obj, ushort ownerPeer = 0)
    {
      unsafe { return NativeSpawn(obj.ObjectID, ownerPeer); }
    }

    /// Host-only push of one component's current state to every replica.
    /// componentKey is the scene codec key ("physics", "render", "network", ...).
    public static bool SyncComponent(SceneObject obj, string componentKey)
    {
      unsafe { return NativeSyncComponent(obj.ObjectID, componentKey); }
    }

    public static bool IsMine(SceneObject obj)
    {
      unsafe { return NativeIsMine(obj.ObjectID); }
    }

    /// Requests a world op. On a client the host validates via OnOpRequest; on the
    /// host it applies directly (same write path, actor 0).
    public static bool RequestOp(string name, ulong subjectNetId = 0, byte[]? payload = null)
    {
      unsafe
      {
        fixed (byte* data = payload)
        {
          return NativeRequestOp(name, subjectNetId, data, payload?.Length ?? 0);
        }
      }
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
    /// Fires on an accepted overlay invite; the engine auto-joins unless
    /// networking.steam.auto-join-invites is off.
    public static Action<ulong>? OnLobbyJoinRequested;
    /// Host-side validator + applier: mutate the world through normal APIs and
    /// return true to accept (state replicates via the snapshot channels). Null =
    /// accept-all (the envelope is audit either way).
    public static Func<ushort, string, ulong, byte[], bool>? OnOpRequest;
    /// Presentation/bookkeeping only on clients — never world mutation.
    public static Action<ushort, string, ulong, byte[]>? OnOpApplied;
    public static Action<string, ushort>? OnOpRejected;

    // native entry points, invoked by name each call so assembly reloads stay safe
    internal static void DispatchPeerJoined(ushort peer) => OnPeerJoined?.Invoke(peer);
    internal static void DispatchPeerLeft(ushort peer) => OnPeerLeft?.Invoke(peer);
    internal static void DispatchLobbyJoinRequested(ulong lobbyId) => OnLobbyJoinRequested?.Invoke(lobbyId);

    internal static void DispatchEvent(ushort sender, string name, int payloadSize)
    {
      if (OnEvent == null)
      {
        return;
      }
      OnEvent.Invoke(sender, name, PullPendingPayload(payloadSize));
    }

    internal static uint DispatchOpRequest(ushort peer, string name, ulong subject, int payloadSize)
    {
      if (OnOpRequest == null)
      {
        return 1;  // accept-all default, matching the join validator
      }
      return OnOpRequest.Invoke(peer, name, subject, PullPendingPayload(payloadSize)) ? 1u : 0u;
    }

    internal static void DispatchOpApplied(ushort actor, string name, ulong subject, int payloadSize)
      => OnOpApplied?.Invoke(actor, name, subject, PullPendingPayload(payloadSize));

    internal static void DispatchOpRejected(string name, ushort reason) => OnOpRejected?.Invoke(name, reason);

    /// The native side parks the payload for the duration of the dispatch call.
    private static byte[] PullPendingPayload(int payloadSize)
    {
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
      return payload;
    }
  }
}
