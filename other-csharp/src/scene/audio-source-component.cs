using System;
using OtherCsBindings;

namespace Other
{
#nullable enable
  [NativeComponent("audio_source_component")]
  public class AudioSourceComponent : Component
  {
    [NativeFunction("GetAudioClipPath")]
    internal static unsafe delegate*<UInt64, NativeString> NativeGetAudioClipPath;
    [NativeFunction("SetAudioClipPath")]
    internal static unsafe delegate*<UInt64, NativeString, void> NativeSetAudioClipPath;

    public AudioSourceComponent(ulong object_id)
      : base(object_id)
    {
    }

    /// clips are assets: the property is the .wav/.mp3 path ("" = no clip).
    public string Clip {
      get
      {
        unsafe
        {
          return NativeGetAudioClipPath(ObjectId).ToString() ?? "";
        }
      }
      set
      {
        unsafe
        {
          NativeSetAudioClipPath(ObjectId, new NativeString(value ?? ""));
        }
      }
    }

    /// desired state: set true to play. flips back to false when a non-looping
    /// voice finishes — poll it to detect completion.
    [NativeField("playing")]
    public bool Playing { get => GetBool(); set => SetBool(value); }
    [NativeField("looping")]
    public bool Looping { get => GetBool(); set => SetBool(value); }
    /// multiplier over the clip's authored default gain.
    [NativeField("volume")]
    public float Volume { get => GetF32(); set => SetF32(value); }
    [NativeField("pitch")]
    public float Pitch { get => GetF32(); set => SetF32(value); }
    /// Audio.Bus values: Master = 0, Music = 1, Sfx = 2, Ui = 3.
    [NativeField("bus")]
    public uint Bus { get => GetU32(); set => SetU32(value); }
    /// false = 2D (ui/music): no attenuation or positional panning.
    [NativeField("spatial")]
    public bool Spatial { get => GetBool(); set => SetBool(value); }
    [NativeField("min_distance")]
    public float MinDistance { get => GetF32(); set => SetF32(value); }
    [NativeField("max_distance")]
    public float MaxDistance { get => GetF32(); set => SetF32(value); }
    [NativeField("doppler_factor")]
    public float DopplerFactor { get => GetF32(); set => SetF32(value); }

    public void Play() => Playing = true;
    public void Stop() => Playing = false;
  }
#nullable disable
}
