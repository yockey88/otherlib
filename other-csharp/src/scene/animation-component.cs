using System;
using OtherCsBindings;

namespace Other
{
#nullable enable
  [NativeComponent("animation_component")]
  public class AnimationComponent : Component
  {
    [NativeFunction("GetAnimationClipPath")]
    internal static unsafe delegate*<UInt64, NativeString> NativeGetAnimationClipPath;
    [NativeFunction("SetAnimationClipPath")]
    internal static unsafe delegate*<UInt64, NativeString, void> NativeSetAnimationClipPath;

    public AnimationComponent(ulong object_id)
      : base(object_id)
    {
    }

    /// clips are assets: the property is the .oanim path ("" = play EmbeddedClip from
    /// the model's own clips instead).
    public string Clip {
      get
      {
        unsafe
        {
          return NativeGetAnimationClipPath(ObjectId).ToString() ?? "";
        }
      }
      set
      {
        unsafe
        {
          NativeSetAnimationClipPath(ObjectId, new NativeString(value ?? ""));
        }
      }
    }

    /// name of a clip embedded in this object's model; only consulted while Clip is "".
    [NativeField("clip_name")]
    public string EmbeddedClip { get => GetNativeString(); set => SetNativeString(value); }

    [NativeField("playing")]
    public bool Playing { get => GetBool(); set => SetBool(value); }
    [NativeField("looping")]
    public bool Looping { get => GetBool(); set => SetBool(value); }
    [NativeField("speed")]
    public float Speed { get => GetF32(); set => SetF32(value); }
    /// seconds into the clip; writable for scrubbing.
    [NativeField("time")]
    public float Time { get => GetF32(); set => SetF32(value); }

    /// play an embedded clip by name from the start.
    public void Play(string name)
    {
      Clip = "";
      EmbeddedClip = name ?? "";
      Time = 0f;
      Playing = true;
    }
  }
#nullable disable
}
