using System;
using System.Runtime.InteropServices;

namespace Other.Core
{
  [Flags]
  public enum BehaviorFieldFlags : uint
  {
    None        = 0,
    ReadOnly    = 1 << 0,
    HasRange    = 1 << 1,
    HasTooltip  = 1 << 2,
    ColorField  = 1 << 3,
    IsGroupStart = 1 << 4,
    HasSeparator = 1 << 5,
    Serializable = 1 << 6,
  }

  [StructLayout(LayoutKind.Sequential)]
  public struct NativeBehaviorFieldDescriptor
  {
    /// value_type enum value matching other::value_type
    public byte field_value_type;

    public BehaviorFieldFlags flags;

    public float range_min;
    public float range_max;
  }
}