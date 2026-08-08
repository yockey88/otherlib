using System;

namespace Other
{
  /// Authored intent: "this is a networked thing". The runtime net id lives in the
  /// scene's network context; Network.IsMine answers ownership questions.
  [NativeComponent("network_component")]
  public class NetworkComponent : Component
  {
    public NetworkComponent(ulong object_id)
      : base(object_id)
    {
    }

    [NativeField("owner_peer")]
    public uint OwnerPeer { get => GetU32(); set => SetU32(value); }

    [NativeField("replicate_transform")]
    public bool ReplicateTransform { get => GetBool(); set => SetBool(value); }

    [NativeField("despawn_on_owner_leave")]
    public bool DespawnOnOwnerLeave { get => GetBool(); set => SetBool(value); }
  }
}
