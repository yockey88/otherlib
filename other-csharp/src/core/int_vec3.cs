using System;
using System.Diagnostics.CodeAnalysis;
using System.Runtime.InteropServices;

namespace Other
{
  [StructLayout(LayoutKind.Sequential)]
  public struct IntVec3 : IEquatable<IntVec3>
  {
    public int X;
    public int Y;
    public int Z;

    public IntVec3(int x, int y, int z)
    {
      X = x;
      Y = y;
      Z = z;
    }

    public static IntVec3 operator+(IntVec3 left, IntVec3 right) => new IntVec3(left.X + right.X, left.Y + right.Y, left.Z + right.Z);
    public static IntVec3 operator-(IntVec3 left, IntVec3 right) => new IntVec3(left.X - right.X, left.Y - right.Y, left.Z - right.Z);
    public static IntVec3 operator*(IntVec3 left, IntVec3 right) => new IntVec3(left.X * right.X, left.Y * right.Y, left.Z * right.Z);
    public static IntVec3 operator/(IntVec3 left, IntVec3 right) => new IntVec3(left.X / right.X, left.Y / right.Y, left.Z / right.Z);
    public static IntVec3 operator+(IntVec3 left, int right) => new IntVec3(left.X + right, left.Y + right, left.Z + right);
    public static IntVec3 operator-(IntVec3 left, int right) => new IntVec3(left.X - right, left.Y - right, left.Z - right);
    public static IntVec3 operator*(IntVec3 left, int right) => new IntVec3(left.X * right, left.Y * right, left.Z * right);
    public static IntVec3 operator/(IntVec3 left, int right) => new IntVec3(left.X / right, left.Y / right, left.Z / right);


    public static bool operator==(IntVec3 left, IntVec3 right) => left.X == right.X && left.Y == right.Y && left.Z == right.Z;
    public static bool operator!=(IntVec3 left, IntVec3 right) => !(left == right);
    public bool Equals(IntVec3 other) => this == other;
    public override bool Equals([NotNullWhen(true)] object obj)
    {
      if (obj is IntVec3 other)
      {
        return this == other;
      }
      return false;
    }
    public override int GetHashCode() => HashCode.Combine(X, Y, Z);
  }
}