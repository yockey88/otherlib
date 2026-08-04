using System;

namespace Other
{
#nullable enable
  /// where 3d audio is heard from; without an active listener the engine falls
  /// back to the "main-camera" tagged object, then the origin.
  [NativeComponent("audio_listener_component")]
  public class AudioListenerComponent : Component
  {
    public AudioListenerComponent(ulong object_id)
      : base(object_id)
    {
    }

    [NativeField("active")]
    public bool Active { get => GetBool(); set => SetBool(value); }
  }
#nullable disable
}
