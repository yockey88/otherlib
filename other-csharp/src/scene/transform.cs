using System;
using OtherCsBindings;

namespace Other
{
  [NativeComponent("transform")]
  public class Transform : Component
  {
    public Transform(ulong object_id)
      : base(object_id)
    {
    }

    [NativeField("local_position")]
    public Vec3 Position { get => GetVec3(); set => SetVec3(value); }

    [NativeField("local_rotation_quat")]
    public Quaternion Rotation { get => GetQuat(); set => SetQuat(value); }

    [NativeField("local_scale")]
    public Vec3 Scale { get => GetVec3(); set => SetVec3(value); }

    public Vec3 EulerAngles
    {
      get => Rotation.ToEulerAngles();
      set
      {
        float deg2rad = MathF.PI / 180.0f;
        Rotation = Quaternion.CreateFromYawPitchRoll(value.Y * deg2rad, value.X * deg2rad, value.Z * deg2rad);
      }
    }

    public void Translate(Vec3 delta)
    {
      Position += delta;
    }
  }
}