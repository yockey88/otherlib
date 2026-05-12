using System;
using OtherCsBindings;

namespace Other
{
  internal static class OtherABI
  {

    [NativeFunction("OeFnvHash")]
    internal static unsafe delegate*<NativeString, UInt64> NativeFnvHash;

    [NativeFunction("OeValidateHandle")]
    internal static unsafe delegate*<UInt64, uint, uint, bool> NativeValidateHandle;
    [NativeFunction("OeHasComponent")]
    internal static unsafe delegate*<UInt64, UInt64, bool> NativeHasComponent;
    [NativeFunction("OeAddComponent")]
    internal static unsafe delegate*<UInt64, UInt64, bool> NativeAddComponent;
    [NativeFunction("OeRemoveComponent")]
    internal static unsafe delegate*<UInt64, UInt64, bool> NativeRemoveComponent;

    [NativeFunction("OeGetFieldBool")]
    internal static unsafe delegate*<UInt64, UInt64, UInt64, bool*, bool> NativeGetFieldBool;
    [NativeFunction("OeSetFieldBool")]
    internal static unsafe delegate*<UInt64, UInt64, UInt64, bool, bool> NativeSetFieldBool;

    [NativeFunction("OeGetFieldI32")]
    internal static unsafe delegate*<UInt64, UInt64, UInt64, int*, bool> NativeGetFieldI32;
    [NativeFunction("OeSetFieldI32")]
    internal static unsafe delegate*<UInt64, UInt64, UInt64, int, bool> NativeSetFieldI32;

    [NativeFunction("OeGetFieldU32")]
    internal static unsafe delegate*<UInt64, UInt64, UInt64, uint*, bool> NativeGetFieldU32;
    [NativeFunction("OeSetFieldU32")]
    internal static unsafe delegate*<UInt64, UInt64, UInt64, uint, bool> NativeSetFieldU32;

    [NativeFunction("OeGetFieldI64")]
    internal static unsafe delegate*<UInt64, UInt64, UInt64, long*, bool> NativeGetFieldI64;
    [NativeFunction("OeSetFieldI64")]
    internal static unsafe delegate*<UInt64, UInt64, UInt64, long, bool> NativeSetFieldI64;

    [NativeFunction("OeGetFieldU64")]
    internal static unsafe delegate*<UInt64, UInt64, UInt64, UInt64*, bool> NativeGetFieldU64;
    [NativeFunction("OeSetFieldU64")]
    internal static unsafe delegate*<UInt64, UInt64, UInt64, UInt64, bool> NativeSetFieldU64;

    [NativeFunction("OeGetFieldF32")]
    internal static unsafe delegate*<UInt64, UInt64, UInt64, float*, bool> NativeGetFieldF32;
    [NativeFunction("OeSetFieldF32")]
    internal static unsafe delegate*<UInt64, UInt64, UInt64, float, bool> NativeSetFieldF32;

    [NativeFunction("OeGetFieldF64")]
    internal static unsafe delegate*<UInt64, UInt64, UInt64, double*, bool> NativeGetFieldF64;
    [NativeFunction("OeSetFieldF64")]
    internal static unsafe delegate*<UInt64, UInt64, UInt64, double, bool> NativeSetFieldF64;

    [NativeFunction("OeGetFieldVec2")]
    internal static unsafe delegate*<UInt64, UInt64, UInt64, float*, float*, bool> NativeGetFieldVec2;
    [NativeFunction("OeSetFieldVec2")]
    internal static unsafe delegate*<UInt64, UInt64, UInt64, float, float, bool> NativeSetFieldVec2;

    [NativeFunction("OeGetFieldVec3")]
    internal static unsafe delegate*<UInt64, UInt64, UInt64, float*, float*, float*, bool> NativeGetFieldVec3;
    [NativeFunction("OeSetFieldVec3")]
    internal static unsafe delegate*<UInt64, UInt64, UInt64, float, float, float, bool> NativeSetFieldVec3;

    [NativeFunction("OeGetFieldVec4")]
    internal static unsafe delegate*<UInt64, UInt64, UInt64, float*, float*, float*, float*, bool> NativeGetFieldVec4;
    [NativeFunction("OeSetFieldVec4")]
    internal static unsafe delegate*<UInt64, UInt64, UInt64, float, float, float, float, bool> NativeSetFieldVec4;

    [NativeFunction("OeGetFieldQuat")]
    internal static unsafe delegate*<UInt64, UInt64, UInt64, float*, float*, float*, float*, bool> NativeGetFieldQuat;
    [NativeFunction("OeSetFieldQuat")]
    internal static unsafe delegate*<UInt64, UInt64, UInt64, float, float, float, float, bool> NativeSetFieldQuat;

    [NativeFunction("OeGetFieldMat4")]
    internal static unsafe delegate*<UInt64, UInt64, UInt64, float*, bool> NativeGetFieldMat4;
    [NativeFunction("OeSetFieldMat4")]
    internal static unsafe delegate*<UInt64, UInt64, UInt64, float*, bool> NativeSetFieldMat4;

    [NativeFunction("OeGetFieldString")]
    internal static unsafe delegate*<UInt64, UInt64, UInt64, NativeString*, bool> NativeGetFieldString;
    [NativeFunction("OeSetFieldString")]
    internal static unsafe delegate*<UInt64, UInt64, UInt64, NativeString, bool> NativeSetFieldString;
  }
}