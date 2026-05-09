using System;
using System.Diagnostics.CodeAnalysis;
using System.Runtime.InteropServices;

namespace Other
{
  [StructLayout(LayoutKind.Sequential)]
  public struct Quaternion : IEquatable<Quaternion>
  {
    public float X;
    public float Y;
    public float Z;
    public float W;

    public Quaternion(float x, float y, float z, float w)
    {
      X = x;
      Y = y;
      Z = z;
      W = w;
    }

    public Vec3 ToEulerAngles()
    {
      // Convert quaternion to Euler angles (in degrees)
      float ysqr = Y * Y;

      // roll (x-axis rotation)
      float t0 = +2.0f * (W * X + Y * Z);
      float t1 = +1.0f - 2.0f * (X * X + ysqr);
      float roll = MathF.Atan2(t0, t1) * (180.0f / MathF.PI);

      // pitch (y-axis rotation)
      float t2 = +2.0f * (W * Y - Z * X);
      t2 = Math.Clamp(t2, -1.0f, 1.0f);
      float pitch = MathF.Asin(t2) * (180.0f / MathF.PI);

      // yaw (z-axis rotation)
      float t3 = +2.0f * (W * Z + X * Y);
      float t4 = +1.0f - 2.0f * (ysqr + Z * Z);
      float yaw = MathF.Atan2(t3, t4) * (180.0f / MathF.PI);

      return new Vec3(pitch, yaw, roll);
    }

    public static Quaternion operator+(Quaternion left, Quaternion right) => new Quaternion(left.X + right.X, left.Y + right.Y, left.Z + right.Z, left.W + right.W);
    public static Quaternion operator-(Quaternion left, Quaternion right) => new Quaternion(left.X - right.X, left.Y - right.Y, left.Z - right.Z, left.W - right.W);
    public static Quaternion operator*(Quaternion left, Quaternion right) => new Quaternion(left.X * right.X, left.Y * right.Y, left.Z * right.Z, left.W * right.W);
    public static Quaternion operator/(Quaternion left, Quaternion right) => new Quaternion(left.X / right.X, left.Y / right.Y, left.Z / right.Z, left.W / right.W);
    public static Quaternion operator+(Quaternion left, float right) => new Quaternion(left.X + right, left.Y + right, left.Z + right, left.W + right);
    public static Quaternion operator-(Quaternion left, float right) => new Quaternion(left.X - right, left.Y - right, left.Z - right, left.W - right);
    public static Quaternion operator*(Quaternion left, float right) => new Quaternion(left.X * right, left.Y * right, left.Z * right, left.W * right);
    public static Quaternion operator/(Quaternion left, float right) => new Quaternion(left.X / right, left.Y / right, left.Z / right, left.W / right);

    public static bool operator==(Quaternion left, Quaternion right) => left.X == right.X && left.Y == right.Y && left.Z == right.Z && left.W == right.W;
    public static bool operator!=(Quaternion left, Quaternion right) => !(left == right);
    public bool Equals(Quaternion other) => this == other;
    public override bool Equals([NotNullWhen(true)] object obj)
    {
      if (obj is Quaternion other)
      {
        return this == other;
      }
      return false;
    }
    public override int GetHashCode() => HashCode.Combine(X, Y, Z, W);

    public static Quaternion CreateFromYawPitchRoll(float yaw, float pitch, float roll)
    {
      float cy = MathF.Cos(yaw * 0.5f);
      float sy = MathF.Sin(yaw * 0.5f);
      float cp = MathF.Cos(pitch * 0.5f);
      float sp = MathF.Sin(pitch * 0.5f);
      float cr = MathF.Cos(roll * 0.5f);
      float sr = MathF.Sin(roll * 0.5f);
      return new Quaternion(
        sr * cp * cy - cr * sp * sy,
        cr * sp * cy + sr * cp * sy,
        cr * cp * sy - sr * sp * cy,
        cr * cp * cy + sr * sp * sy
      );
    }
  }
}