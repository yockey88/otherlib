using System;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.CompilerServices;
using OtherCsBindings;

namespace Other
{
  public class Component
  {
    private struct FieldKey
    {
      public Type type;
      public string managed_name;
    }
    private struct FieldInfo
    {
      public string native_name;
      public UInt64 native_id;
    }

    protected ulong object_id;
    protected readonly ulong component_id;

    private static readonly Dictionary<Type, ulong> component_ids = new();
    private static readonly Dictionary<FieldKey, FieldInfo> field_maps = new();

    public UInt64 ObjectId => object_id;

    protected Component(ulong object_id) 
    {
      this.object_id = object_id;
      component_id = TypeId(GetType());    
    }
    
    static public UInt64 TypeId<T>() 
      where T : Component
    {
      return TypeId(typeof(T));
    }

    static private UInt64 TypeId(Type type) 
    {
      if (!component_ids.TryGetValue(type, out var cid))
      {
        CacheType(type);
        cid = component_ids[type];
      }
      return cid;
    }

    private static void CacheType(Type type)
    {
      var comp_attr = (NativeComponentAttribute)Attribute.GetCustomAttribute(type, typeof(NativeComponentAttribute));
      if (comp_attr == null)
      {
        Logger.LogError($"Component class '{type.Name}' is missing the [NativeComponent] attribute.");
        throw new InvalidOperationException($"Component class '{type.Name}' is missing the [NativeComponent] attribute.");
      }
      component_ids[type] = comp_attr.Id;

      foreach (var prop in type.GetProperties(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance))
      {
        var field_attr = (NativeFieldAttribute)Attribute.GetCustomAttribute(prop, typeof(NativeFieldAttribute));
        if (field_attr == null)
        {
          continue;
        }
        field_maps[new FieldKey { type = type, managed_name = prop.Name }] = new FieldInfo { native_name = field_attr.Name, native_id = field_attr.Id };
      }
    }

    private ulong FieldId([CallerMemberName] string prop = "")
    {
      if (field_maps.TryGetValue(new FieldKey { type = GetType(), managed_name = prop }, out var fi))
      {
        return fi.native_id;
      }
      throw new InvalidOperationException(
        $"Property '{prop}' on '{GetType().Name}' has no [NativeField] mapping.");
    }

    protected bool GetBool([CallerMemberName] string prop = "")
    {
      bool v = false;
      unsafe { OtherABI.NativeGetFieldBool(object_id, component_id, FieldId(prop), &v); }
      return v;
    }

    protected void SetBool(bool value, [CallerMemberName] string prop = "")
    {
      unsafe { OtherABI.NativeSetFieldBool(object_id, component_id, FieldId(prop), value); }
    }

    protected int GetI32([CallerMemberName] string prop = "")
    {
      int v = 0;
      unsafe { OtherABI.NativeGetFieldI32(object_id, component_id, FieldId(prop), &v); }
      return v;
    }

    protected void SetI32(int value, [CallerMemberName] string prop = "")
    {
      unsafe { OtherABI.NativeSetFieldI32(object_id, component_id, FieldId(prop), value); }
    }

    protected uint GetU32([CallerMemberName] string prop = "")
    {
      uint v = 0;
      unsafe { OtherABI.NativeGetFieldU32(object_id, component_id, FieldId(prop), &v); }
      return v;
    }

    protected void SetU32(uint value, [CallerMemberName] string prop = "")
    {
      unsafe { OtherABI.NativeSetFieldU32(object_id, component_id, FieldId(prop), value); }
    }

    protected long GetI64([CallerMemberName] string prop = "")
    {
      long v = 0;
      unsafe { OtherABI.NativeGetFieldI64(object_id, component_id, FieldId(prop), &v); }
      return v;
    }

    protected void SetI64(long value, [CallerMemberName] string prop = "")
    {
      unsafe { OtherABI.NativeSetFieldI64(object_id, component_id, FieldId(prop), value); }
    }

    protected ulong GetU64([CallerMemberName] string prop = "")
    {
      ulong v = 0;
      unsafe { OtherABI.NativeGetFieldU64(object_id, component_id, FieldId(prop), &v); }
      return v;
    }

    protected void SetU64(ulong value, [CallerMemberName] string prop = "")
    {
      unsafe { OtherABI.NativeSetFieldU64(object_id, component_id, FieldId(prop), value); }
    }

    protected float GetF32([CallerMemberName] string prop = "")
    {
      float v = 0;
      unsafe { OtherABI.NativeGetFieldF32(object_id, component_id, FieldId(prop), &v); }
      return v;
    }

    protected void SetF32(float value, [CallerMemberName] string prop = "")
    {
      unsafe { OtherABI.NativeSetFieldF32(object_id, component_id, FieldId(prop), value); }
    }

    protected double GetF64([CallerMemberName] string prop = "")
    {
      double v = 0;
      unsafe { OtherABI.NativeGetFieldF64(object_id, component_id, FieldId(prop), &v); }
      return v;
    }

    protected void SetF64(double value, [CallerMemberName] string prop = "")
    {
      unsafe { OtherABI.NativeSetFieldF64(object_id, component_id, FieldId(prop), value); }
    }

    protected Vec2 GetVec2([CallerMemberName] string prop = "")
    {
      float x = 0, y = 0;
      unsafe { OtherABI.NativeGetFieldVec2(object_id, component_id, FieldId(prop), &x, &y); }
      return new Vec2(x, y);
    }

    protected void SetVec2(Vec2 value, [CallerMemberName] string prop = "")
    {
      unsafe { OtherABI.NativeSetFieldVec2(object_id, component_id, FieldId(prop), value.X, value.Y); }
    }

    protected Vec3 GetVec3([CallerMemberName] string prop = "")
    {
      float x = 0, y = 0, z = 0;
      unsafe { OtherABI.NativeGetFieldVec3(object_id, component_id, FieldId(prop), &x, &y, &z); }
      return new Vec3(x, y, z);
    }

    protected void SetVec3(Vec3 value, [CallerMemberName] string prop = "")
    {
      unsafe { 
        OtherABI.NativeSetFieldVec3(object_id, component_id, FieldId(prop), value.X, value.Y, value.Z); 
      }
    }

    protected Vec4 GetVec4([CallerMemberName] string prop = "")
    {
      float x = 0, y = 0, z = 0, w = 0;
      unsafe { OtherABI.NativeGetFieldVec4(object_id, component_id, FieldId(prop), &x, &y, &z, &w); }
      return new Vec4(x, y, z, w);
    }

    protected void SetVec4(Vec4 value, [CallerMemberName] string prop = "")
    {
      unsafe { OtherABI.NativeSetFieldVec4(object_id, component_id, FieldId(prop), value.X, value.Y, value.Z, value.W); }
    }

    protected Quaternion GetQuat([CallerMemberName] string prop = "")
    {
      float x = 0, y = 0, z = 0, w = 0;
      unsafe { OtherABI.NativeGetFieldQuat(object_id, component_id, FieldId(prop), &x, &y, &z, &w); }
      return new Quaternion(x, y, z, w);
    }

    protected void SetQuat(Quaternion value, [CallerMemberName] string prop = "")
    {
      unsafe { OtherABI.NativeSetFieldQuat(object_id, component_id, FieldId(prop), value.X, value.Y, value.Z, value.W); }
    }

    protected Mat4 GetMat4([CallerMemberName] string prop = "")
    {
      unsafe
      {
        float* ptr = stackalloc float[16];
        OtherABI.NativeGetFieldMat4(object_id, component_id, FieldId(prop), ptr);
        return new Mat4(
          ptr[0], ptr[1], ptr[2], ptr[3],
          ptr[4], ptr[5], ptr[6], ptr[7],
          ptr[8], ptr[9], ptr[10], ptr[11],
          ptr[12], ptr[13], ptr[14], ptr[15]
        );
      }
    }

    protected void SetMat4(Mat4 value, [CallerMemberName] string prop = "")
    {
      unsafe
      {
        float* ptr = stackalloc float[16];
        ptr[0] = value.M11; ptr[1] = value.M12; ptr[2] = value.M13; ptr[3] = value.M14;
        ptr[4] = value.M21; ptr[5] = value.M22; ptr[6] = value.M23; ptr[7] = value.M24;
        ptr[8] = value.M31; ptr[9] = value.M32; ptr[10] = value.M33; ptr[11] = value.M34;
        ptr[12] = value.M41; ptr[13] = value.M42; ptr[14] = value.M43; ptr[15] = value.M44;
        OtherABI.NativeSetFieldMat4(object_id, component_id, FieldId(prop), ptr);
      }
    }

    protected string GetNativeString([CallerMemberName] string prop = "")
    {
      unsafe
      {
        NativeString ns = default;
        OtherABI.NativeGetFieldString(object_id, component_id, FieldId(prop), &ns);
        return ns.ToString();
      }
    }

    protected void SetNativeString(string value, [CallerMemberName] string prop = "")
    {
      unsafe { OtherABI.NativeSetFieldString(object_id, component_id, FieldId(prop), value); }
    }
  }  
}