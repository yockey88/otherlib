using System;
using System.Diagnostics.CodeAnalysis;
using System.Runtime.InteropServices;

namespace Other
{
  [StructLayout(LayoutKind.Sequential)]
  public struct IntVec2 : IEquatable<IntVec2>
  {
    public int X;
    public int Y;

    public IntVec2(int x, int y)
    {
      X = x;
      Y = y;
    }

    public static IntVec2 operator+(IntVec2 left, IntVec2 right) => new IntVec2(left.X + right.X, left.Y + right.Y);
    public static IntVec2 operator-(IntVec2 left, IntVec2 right) => new IntVec2(left.X - right.X, left.Y - right.Y);
    public static IntVec2 operator*(IntVec2 left, IntVec2 right) => new IntVec2(left.X * right.X, left.Y * right.Y);
    public static IntVec2 operator/(IntVec2 left, IntVec2 right) => new IntVec2(left.X / right.X, left.Y / right.Y);
    public static IntVec2 operator+(IntVec2 left, int right) => new IntVec2(left.X + right, left.Y + right);
    public static IntVec2 operator-(IntVec2 left, int right) => new IntVec2(left.X - right, left.Y - right);
    public static IntVec2 operator*(IntVec2 left, int right) => new IntVec2(left.X * right, left.Y * right);
    public static IntVec2 operator/(IntVec2 left, int right) => new IntVec2(left.X / right, left.Y / right);

    public static bool operator==(IntVec2 left, IntVec2 right) => left.X == right.X && left.Y == right.Y;
    public static bool operator!=(IntVec2 left, IntVec2 right) => !(left == right);
    public bool Equals(IntVec2 other) => this == other;
    public override bool Equals([NotNullWhen(true)] object obj)
    {
      if (obj is IntVec2 other)
      {
        return this == other;
      }
      return false;
    }
    public override int GetHashCode() => HashCode.Combine(X, Y);
  }
}