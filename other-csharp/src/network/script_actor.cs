using System;
using OtherCsBindings;

namespace Other.Networking
{
  /// A managed peer-actor behavior. Bind an implementation by setting
  /// networking.session-host = "script:Namespace.Type, Assembly" — the engine
  /// instantiates the type and routes the mesh callbacks here.
  public interface IScriptActor
  {
    void OnFrame(ulong srcNode, uint netId, byte[] payload);
    void OnLinkUp(ulong linkId, ulong remoteNode);
    void OnLinkDown(ulong linkId, uint reason);
    void Tick(double dt);
  }

  /// The native bridge for the bound IScriptActor: dispatch entries the engine calls
  /// by name, and the actor-side seat operations (send/open/close).
  public static class ScriptActor
  {
    [NativeFunction("NetworkActorStagePayload")]
    internal static unsafe delegate*<byte*, int, void> NativeStagePayload;
    [NativeFunction("NetworkActorSend")]
    internal static unsafe delegate*<ulong, uint, NativeBool32> NativeSend;
    [NativeFunction("NetworkActorOpenLink")]
    internal static unsafe delegate*<NativeString, ushort, NativeString, ulong> NativeOpenLink;
    [NativeFunction("NetworkActorOpenListener")]
    internal static unsafe delegate*<ushort, NativeString, ulong> NativeOpenListener;
    [NativeFunction("NetworkActorCloseLink")]
    internal static unsafe delegate*<ulong, uint, void> NativeCloseLink;

    private static IScriptActor? active;

    /// The bound behavior instance; assignable directly for tests or manual wiring.
    public static IScriptActor? Active
    {
      get => active;
      set => active = value;
    }

    public static bool Send(ulong dstNode, uint netId, byte[]? payload = null)
    {
      unsafe
      {
        fixed (byte* data = payload)
        {
          NativeStagePayload(data, payload?.Length ?? 0);
        }
        return NativeSend(dstNode, netId);
      }
    }

    /// transport "" resolves by address kind (tcp for ip); pass "udp" to override
    public static ulong OpenLink(string address, ushort port, string transport = "")
    {
      unsafe { return NativeOpenLink(address, port, transport); }
    }

    public static ulong OpenListener(ushort port, string transport = "")
    {
      unsafe { return NativeOpenListener(port, transport); }
    }

    public static void CloseLink(ulong linkId, uint reason = 0)
    {
      unsafe { NativeCloseLink(linkId, reason); }
    }

    /// ---------------------------------------------------------- engine dispatch

    internal static void Bind(string typeName)
    {
      var type = Type.GetType(typeName);
      if (type == null)
      {
        Console.Error.WriteLine($"[ScriptActor] type '{typeName}' not found");
        return;
      }
      active = Activator.CreateInstance(type) as IScriptActor;
      if (active == null)
      {
        Console.Error.WriteLine($"[ScriptActor] type '{typeName}' does not implement IScriptActor");
      }
    }

    internal static void DispatchActorFrame(ulong srcNode, uint netId, int payloadSize)
      => active?.OnFrame(srcNode, netId, Network.PullPendingPayload(payloadSize));

    internal static void DispatchActorLinkUp(ulong linkId, ulong remoteNode)
      => active?.OnLinkUp(linkId, remoteNode);

    internal static void DispatchActorLinkDown(ulong linkId, uint reason)
      => active?.OnLinkDown(linkId, reason);

    internal static void DispatchActorTick(double dt)
      => active?.Tick(dt);
  }
}
