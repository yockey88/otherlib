using System;
using System.Diagnostics.CodeAnalysis;
using System.Runtime.InteropServices;

namespace Other
{
  [StructLayout(LayoutKind.Sequential)]
  public struct Vec2 : IEquatable<Vec2>
  {
    public float X;
    public float Y;

    public Vec2(float x, float y)
    {
      X = x;
      Y = y;
    }

    public static Vec2 operator+(Vec2 left, Vec2 right) => new Vec2(left.X + right.X, left.Y + right.Y);
    public static Vec2 operator-(Vec2 left, Vec2 right) => new Vec2(left.X - right.X, left.Y - right.Y);
    public static Vec2 operator*(Vec2 left, Vec2 right) => new Vec2(left.X * right.X, left.Y * right.Y);
    public static Vec2 operator/(Vec2 left, Vec2 right) => new Vec2(left.X / right.X, left.Y / right.Y);
    public static Vec2 operator+(Vec2 left, float right) => new Vec2(left.X + right, left.Y + right);
    public static Vec2 operator-(Vec2 left, float right) => new Vec2(left.X - right, left.Y - right);
    public static Vec2 operator*(Vec2 left, float right) => new Vec2(left.X * right, left.Y * right);
    public static Vec2 operator/(Vec2 left, float right) => new Vec2(left.X / right, left.Y / right);

    public static bool operator==(Vec2 left, Vec2 right) => left.X == right.X && left.Y == right.Y;
    public static bool operator!=(Vec2 left, Vec2 right) => !(left == right);
    public bool Equals(Vec2 other) => this == other;
    public override bool Equals([NotNullWhen(true)] object obj)
    {
      if (obj is Vec2 other)
      {
        return this == other;
      }
      return false;
    }
    public override int GetHashCode() => HashCode.Combine(X, Y);
  }

  [StructLayout(LayoutKind.Sequential)]
  public struct Vec3 : IEquatable<Vec3>
  {
    public float X;
    public float Y;
    public float Z;

    public Vec3(float x, float y, float z)
    {
      X = x;
      Y = y;
      Z = z;
    }

    public static Vec3 operator+(Vec3 left, Vec3 right) => new Vec3(left.X + right.X, left.Y + right.Y, left.Z + right.Z);
    public static Vec3 operator-(Vec3 left, Vec3 right) => new Vec3(left.X - right.X, left.Y - right.Y, left.Z - right.Z);
    public static Vec3 operator*(Vec3 left, Vec3 right) => new Vec3(left.X * right.X, left.Y * right.Y, left.Z * right.Z);
    public static Vec3 operator/(Vec3 left, Vec3 right) => new Vec3(left.X / right.X, left.Y / right.Y, left.Z / right.Z);
    public static Vec3 operator+(Vec3 left, float right) => new Vec3(left.X + right, left.Y + right, left.Z + right);
    public static Vec3 operator-(Vec3 left, float right) => new Vec3(left.X - right, left.Y - right, left.Z - right);
    public static Vec3 operator*(Vec3 left, float right) => new Vec3(left.X * right, left.Y * right, left.Z * right);
    public static Vec3 operator/(Vec3 left, float right) => new Vec3(left.X / right, left.Y / right, left.Z / right);


    public static bool operator==(Vec3 left, Vec3 right) => left.X == right.X && left.Y == right.Y && left.Z == right.Z;
    public static bool operator!=(Vec3 left, Vec3 right) => !(left == right);
    public bool Equals(Vec3 other) => this == other;
    public override bool Equals([NotNullWhen(true)] object obj)
    {
      if (obj is Vec3 other)
      {
        return this == other;
      }
      return false;
    }
    public override int GetHashCode() => HashCode.Combine(X, Y, Z);
  }

  [StructLayout(LayoutKind.Sequential)]
  public struct Vec4 : IEquatable<Vec4>
  {
    public float X;
    public float Y;
    public float Z;
    public float W;

    public Vec4(float x, float y, float z, float w)
    {
      X = x;
      Y = y;
      Z = z;
      W = w;
    }

    public static Vec4 operator+(Vec4 left, Vec4 right) => new Vec4(left.X + right.X, left.Y + right.Y, left.Z + right.Z, left.W + right.W);
    public static Vec4 operator-(Vec4 left, Vec4 right) => new Vec4(left.X - right.X, left.Y - right.Y, left.Z - right.Z, left.W - right.W);
    public static Vec4 operator*(Vec4 left, Vec4 right) => new Vec4(left.X * right.X, left.Y * right.Y, left.Z * right.Z, left.W * right.W);
    public static Vec4 operator/(Vec4 left, Vec4 right) => new Vec4(left.X / right.X, left.Y / right.Y, left.Z / right.Z, left.W / right.W);
    public static Vec4 operator+(Vec4 left, float right) => new Vec4(left.X + right, left.Y + right, left.Z + right, left.W + right);
    public static Vec4 operator-(Vec4 left, float right) => new Vec4(left.X - right, left.Y - right, left.Z - right, left.W - right);
    public static Vec4 operator*(Vec4 left, float right) => new Vec4(left.X * right, left.Y * right, left.Z * right, left.W * right);
    public static Vec4 operator/(Vec4 left, float right) => new Vec4(left.X / right, left.Y / right, left.Z / right, left.W / right);

    public static bool operator==(Vec4 left, Vec4 right) => left.X == right.X && left.Y == right.Y && left.Z == right.Z && left.W == right.W;
    public static bool operator!=(Vec4 left, Vec4 right) => !(left == right);
    public bool Equals(Vec4 other) => this == other;
    public override bool Equals([NotNullWhen(true)] object obj)
    {
      if (obj is Vec4 other)
      {
        return this == other;
      }
      return false;
    }
    public override int GetHashCode() => HashCode.Combine(X, Y, Z, W);
  }

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