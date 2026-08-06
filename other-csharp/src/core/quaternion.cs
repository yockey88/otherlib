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

    public static readonly Quaternion Identity = new Quaternion(0f, 0f, 0f, 1f);

    public static Quaternion operator+(Quaternion left, Quaternion right) => new Quaternion(left.X + right.X, left.Y + right.Y, left.Z + right.Z, left.W + right.W);
    public static Quaternion operator-(Quaternion left, Quaternion right) => new Quaternion(left.X - right.X, left.Y - right.Y, left.Z - right.Z, left.W - right.W);
    /// Hamilton product: rotation by right, then by left
    public static Quaternion operator*(Quaternion left, Quaternion right) => new Quaternion(
      left.W * right.X + left.X * right.W + left.Y * right.Z - left.Z * right.Y,
      left.W * right.Y - left.X * right.Z + left.Y * right.W + left.Z * right.X,
      left.W * right.Z + left.X * right.Y - left.Y * right.X + left.Z * right.W,
      left.W * right.W - left.X * right.X - left.Y * right.Y - left.Z * right.Z
    );
    public static Quaternion operator/(Quaternion left, Quaternion right) => left * right.Inverse();
    public static Quaternion operator*(Quaternion left, float right) => new Quaternion(left.X * right, left.Y * right, left.Z * right, left.W * right);
    public static Quaternion operator/(Quaternion left, float right) => new Quaternion(left.X / right, left.Y / right, left.Z / right, left.W / right);

    /// rotates a vector by this quaternion (assumes unit length)
    public static Vec3 operator*(Quaternion q, Vec3 v)
    {
      // v' = v + 2w(u x v) + 2(u x (u x v)), u = (X, Y, Z)
      Vec3 u = new Vec3(q.X, q.Y, q.Z);
      Vec3 uxv = Vec3.Cross(u, v);
      Vec3 uxuxv = Vec3.Cross(u, uxv);
      return v + (uxv * (2.0f * q.W)) + (uxuxv * 2.0f);
    }

    public float LengthSquared() => X * X + Y * Y + Z * Z + W * W;
    public float Length() => MathF.Sqrt(LengthSquared());

    public Quaternion Conjugate() => new Quaternion(-X, -Y, -Z, W);

    public Quaternion Inverse()
    {
      float len_sq = LengthSquared();
      if (len_sq <= float.Epsilon)
      {
        return Identity;
      }
      Quaternion conj = Conjugate();
      return conj / len_sq;
    }

    public Quaternion Normalized()
    {
      float len = Length();
      if (len <= float.Epsilon)
      {
        return Identity;
      }
      return this / len;
    }

    public static float Dot(Quaternion a, Quaternion b) => a.X * b.X + a.Y * b.Y + a.Z * b.Z + a.W * b.W;

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