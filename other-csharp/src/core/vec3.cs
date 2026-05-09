using System;
using System.Diagnostics.CodeAnalysis;
using System.Runtime.InteropServices;

namespace Other
{
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

    public static Vec3 Zero => new Vec3(0f, 0f, 0f);
    public static Vec3 One => new Vec3(1f, 1f, 1f);
    public static Vec3 Up => new Vec3(0f, 1f, 0f);
    public static Vec3 Down => new Vec3(0f, -1f, 0f);
    public static Vec3 Left => new Vec3(-1f, 0f, 0f);
    public static Vec3 Right => new Vec3(1f, 0f, 0f);
    public static Vec3 Forward => new Vec3(0f, 0f, 1f);
    public static Vec3 Backward => new Vec3(0f, 0f, -1f);
  }
}