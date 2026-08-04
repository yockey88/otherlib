using System;
using OtherCsBindings;

namespace Other
{
  /// engine-level audio: fire-and-forget one-shots and bus volumes. per-object
  /// playback lives on AudioSourceComponent.
  public static class Audio
  {
    public const uint MasterBus = 0;
    public const uint MusicBus = 1;
    public const uint SfxBus = 2;
    public const uint UiBus = 3;

    [NativeFunction("AudioPlayOneShot")]
    internal static unsafe delegate*<NativeString, float, float, float, float, float, uint, void> NativePlayOneShot;
    [NativeFunction("AudioSetBusVolume")]
    internal static unsafe delegate*<uint, float, void> NativeSetBusVolume;
    [NativeFunction("AudioGetBusVolume")]
    internal static unsafe delegate*<uint, float> NativeGetBusVolume;

    /// best effort: the clip must be a loaded AUDIO asset (scene-referenced clips
    /// are; a first-ever call may only kick the load and stay silent).
    public static void PlayOneShot(string clip, Vec3 position, float volume = 1f, float pitch = 1f, uint bus = SfxBus)
    {
      unsafe { NativePlayOneShot(new NativeString(clip ?? ""), position.X, position.Y, position.Z, volume, pitch, bus); }
    }

    public static void SetBusVolume(uint bus, float volume)
    {
      unsafe { NativeSetBusVolume(bus, volume); }
    }

    public static float GetBusVolume(uint bus)
    {
      unsafe { return NativeGetBusVolume(bus); }
    }
  }
}
