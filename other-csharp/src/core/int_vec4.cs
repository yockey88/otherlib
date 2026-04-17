using System;
using System.Diagnostics.CodeAnalysis;
using System.Runtime.InteropServices;

namespace Other
{
  [StructLayout(LayoutKind.Sequential)]
  public struct IntVec4 : IEquatable<IntVec4>
  {
    public int X;
    public int Y;
    public int Z;
    public int W;

    public IntVec4(int x, int y, int z, int w)
    {
      X = x;
      Y = y;
      Z = z;
      W = w;
    }

    public static IntVec4 operator+(IntVec4 left, IntVec4 right) => new IntVec4(left.X + right.X, left.Y + right.Y, left.Z + right.Z, left.W + right.W);
    public static IntVec4 operator-(IntVec4 left, IntVec4 right) => new IntVec4(left.X - right.X, left.Y - right.Y, left.Z - right.Z, left.W - right.W);
    public static IntVec4 operator*(IntVec4 left, IntVec4 right) => new IntVec4(left.X * right.X, left.Y * right.Y, left.Z * right.Z, left.W * right.W);
    public static IntVec4 operator/(IntVec4 left, IntVec4 right) => new IntVec4(left.X / right.X, left.Y / right.Y, left.Z / right.Z, left.W / right.W);
    public static IntVec4 operator+(IntVec4 left, int right) => new IntVec4(left.X + right, left.Y + right, left.Z + right, left.W + right);
    public static IntVec4 operator-(IntVec4 left, int right) => new IntVec4(left.X - right, left.Y - right, left.Z - right, left.W - right);
    public static IntVec4 operator*(IntVec4 left, int right) => new IntVec4(left.X * right, left.Y * right, left.Z * right, left.W * right);
    public static IntVec4 operator/(IntVec4 left, int right) => new IntVec4(left.X / right, left.Y / right, left.Z / right, left.W / right);

    public static bool operator==(IntVec4 left, IntVec4 right) => left.X == right.X && left.Y == right.Y && left.Z == right.Z && left.W == right.W;
    public static bool operator!=(IntVec4 left, IntVec4 right) => !(left == right);
    public bool Equals(IntVec4 other) => this == other;
    public override bool Equals([NotNullWhen(true)] object obj)
    {
      if (obj is IntVec4 other)
      {
        return this == other;
      }
      return false;
    }
    public override int GetHashCode() => HashCode.Combine(X, Y, Z, W);
  }
}