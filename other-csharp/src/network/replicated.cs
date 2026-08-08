using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using Other.Core;

namespace Other.Networking
{
#nullable enable
  /// Host-authoritative field sync, swept at snapshot-hz and shipped to replicas. Fields only
  /// (value types/enums/strings); a client-side write is lost at the next sweep.
  [AttributeUsage(AttributeTargets.Field)]
  public class ReplicatedAttribute : Attribute
  {
  }

  /// Instance tracker + wire codec for [Replicated] fields; native just transports the blob
  /// keyed by net id. Reload-safe: instances re-track on AddBehavior, dirty cache dies with them.
  internal static class ReplicatedSync
  {
    private static readonly Dictionary<Type, FieldInfo[]> fields_by_type = new();
    private static readonly List<OtherBehavior> tracked = new();
    private static readonly Dictionary<OtherBehavior, object?[]> last_sent = new();

    internal static void Track(OtherBehavior behavior)
    {
      FieldInfo[] fields = ReplicatedFieldsOf(behavior.GetType());
      if (fields.Length == 0 || tracked.Contains(behavior))
      {
        return;
      }
      tracked.Add(behavior);
      last_sent[behavior] = new object?[fields.Length];
    }

    internal static void Untrack(OtherBehavior behavior)
    {
      tracked.Remove(behavior);
      last_sent.Remove(behavior);
    }

    /// Serializes every dirty [Replicated] field on the object's behaviors;
    /// null when nothing changed.
    internal static byte[]? Collect(ulong objectId)
    {
      MemoryStream? stream = null;
      BinaryWriter? writer = null;
      byte entry_count = 0;

      foreach (OtherBehavior behavior in tracked)
      {
        if (behavior.ObjectID != objectId)
        {
          continue;
        }
        FieldInfo[] fields = ReplicatedFieldsOf(behavior.GetType());
        object?[] cache = last_sent[behavior];
        for (int i = 0; i < fields.Length; ++i)
        {
          object? value = fields[i].GetValue(behavior);
          if (Equals(value, cache[i]))
          {
            continue;
          }
          cache[i] = value;
          if (writer == null)
          {
            stream = new MemoryStream();
            writer = new BinaryWriter(stream);
            writer.Write((byte)0);  // count patched below
          }
          writer.Write(behavior.GetType().FullName ?? "");
          writer.Write(fields[i].Name);
          WriteValue(writer, fields[i].FieldType, value);
          entry_count++;
        }
      }

      if (writer == null || stream == null)
      {
        return null;
      }
      byte[] blob = stream.ToArray();
      blob[0] = entry_count;
      return blob;
    }

    internal static void Apply(ulong objectId, byte[] payload)
    {
      using var reader = new BinaryReader(new MemoryStream(payload));
      byte count = reader.ReadByte();
      for (int entry = 0; entry < count; ++entry)
      {
        string behavior_name = reader.ReadString();
        string field_name = reader.ReadString();
        object? value = ReadValue(reader);

        foreach (OtherBehavior behavior in tracked)
        {
          if (behavior.ObjectID != objectId || behavior.GetType().FullName != behavior_name)
          {
            continue;
          }
          FieldInfo[] fields = ReplicatedFieldsOf(behavior.GetType());
          for (int i = 0; i < fields.Length; ++i)
          {
            if (fields[i].Name != field_name)
            {
              continue;
            }
            Type target = fields[i].FieldType;
            object? converted = value != null && target.IsEnum ? Enum.ToObject(target, value)
                              : value != null && !target.IsInstanceOfType(value) ? Convert.ChangeType(value, target)
                              : value;
            fields[i].SetValue(behavior, converted);
            last_sent[behavior][i] = converted;
            break;
          }
          break;
        }
      }
    }

    private static FieldInfo[] ReplicatedFieldsOf(Type type)
    {
      if (fields_by_type.TryGetValue(type, out FieldInfo[]? cached))
      {
        return cached;
      }
      var found = new List<FieldInfo>();
      foreach (FieldInfo field in type.GetFields(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic))
      {
        if (field.GetCustomAttribute<ReplicatedAttribute>() != null)
        {
          found.Add(field);
        }
      }
      FieldInfo[] fields = found.ToArray();
      fields_by_type[type] = fields;
      return fields;
    }

    private static void WriteValue(BinaryWriter writer, Type declared, object? value)
    {
      Type type = declared.IsEnum ? Enum.GetUnderlyingType(declared) : declared;
      object? plain = value != null && declared.IsEnum ? Convert.ChangeType(value, type) : value;
      switch (Type.GetTypeCode(type))
      {
        case TypeCode.Boolean: writer.Write((byte)1); writer.Write(plain != null && (bool)plain); break;
        case TypeCode.SByte: writer.Write((byte)2); writer.Write(plain != null ? (sbyte)plain : (sbyte)0); break;
        case TypeCode.Byte: writer.Write((byte)3); writer.Write(plain != null ? (byte)plain : (byte)0); break;
        case TypeCode.Int16: writer.Write((byte)4); writer.Write(plain != null ? (short)plain : (short)0); break;
        case TypeCode.UInt16: writer.Write((byte)5); writer.Write(plain != null ? (ushort)plain : (ushort)0); break;
        case TypeCode.Int32: writer.Write((byte)6); writer.Write(plain != null ? (int)plain : 0); break;
        case TypeCode.UInt32: writer.Write((byte)7); writer.Write(plain != null ? (uint)plain : 0u); break;
        case TypeCode.Int64: writer.Write((byte)8); writer.Write(plain != null ? (long)plain : 0L); break;
        case TypeCode.UInt64: writer.Write((byte)9); writer.Write(plain != null ? (ulong)plain : 0UL); break;
        case TypeCode.Single: writer.Write((byte)10); writer.Write(plain != null ? (float)plain : 0f); break;
        case TypeCode.Double: writer.Write((byte)11); writer.Write(plain != null ? (double)plain : 0d); break;
        case TypeCode.String: writer.Write((byte)12); writer.Write(plain as string ?? ""); break;
        default: writer.Write((byte)0); break;  // unsupported: shipped as "no value"
      }
    }

    private static object? ReadValue(BinaryReader reader)
    {
      byte tag = reader.ReadByte();
      return tag switch
      {
        1 => reader.ReadBoolean(),
        2 => reader.ReadSByte(),
        3 => reader.ReadByte(),
        4 => reader.ReadInt16(),
        5 => reader.ReadUInt16(),
        6 => reader.ReadInt32(),
        7 => reader.ReadUInt32(),
        8 => reader.ReadInt64(),
        9 => reader.ReadUInt64(),
        10 => reader.ReadSingle(),
        11 => reader.ReadDouble(),
        12 => reader.ReadString(),
        _ => null,
      };
    }
  }
}
