using System;
using System.Diagnostics.CodeAnalysis;
using System.Runtime.InteropServices;

namespace Other
{
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
}