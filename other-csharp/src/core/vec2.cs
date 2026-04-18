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
}