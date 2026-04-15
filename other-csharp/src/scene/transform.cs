using System;

namespace Other
{
  public class Transform
  {
    private ulong object_id;

    internal Transform(ulong object_id)
    {
      this.object_id = object_id;
    }

    public Vec3 Position
    {
      get => TransformAccess.GetPosition(object_id);
      set => TransformAccess.SetPosition(object_id, value);
    }

    public Quaternion Rotation
    {
      get => TransformAccess.GetRotation(object_id);
      set => TransformAccess.SetRotation(object_id, value);
    }

    public Vec3 Scale
    {
      get => TransformAccess.GetScale(object_id);
      set => TransformAccess.SetScale(object_id, value);
    }

    public Mat4 WorldMatrix => TransformAccess.GetWorldMatrix(object_id);
    public Vec3 EulerAngles
    {
      set
      {
        float deg2rad = MathF.PI / 180.0f;
        Rotation = Quaternion.CreateFromYawPitchRoll(
          value.Y * deg2rad,
          value.X * deg2rad,
          value.Z * deg2rad
        );
      }
    }

    public void Translate(Vec3 delta)
    {
      Position += delta;
    }
  }
}