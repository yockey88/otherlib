using System;
using OtherCsBindings;

namespace Other.Core
{
  /// <summary>
  /// Provides access to add, remove, and query components on scene objects by name.
  /// Components are identified by their registered type name in the engine's type database.
  /// </summary>
  public static class Components
  {
    [NativeFunction("ComponentAddByName")]
    internal static unsafe delegate*<UInt64, NativeString, void> NativeAddByName;
    [NativeFunction("ComponentRemoveByName")]
    internal static unsafe delegate*<UInt64, NativeString, void> NativeRemoveByName;
    [NativeFunction("ComponentHasByName")]
    internal static unsafe delegate*<UInt64, NativeString, NativeBool32> NativeHasByName;

    public static void Add(ulong object_id, string component_name)
    {
      unsafe { NativeAddByName(object_id, component_name); }
    }

    public static void Remove(ulong object_id, string component_name)
    {
      unsafe { NativeRemoveByName(object_id, component_name); }
    }

    public static bool Has(ulong object_id, string component_name)
    {
      unsafe { return NativeHasByName(object_id, component_name); }
    }

    public static class TypeNames
    {
      public const string Transform = "transform";
      public const string Renderer = "render_component";
      public const string Physics = "physics_component";
      public const string Script = "script_component";
      public const string Audio = "audio_component";
      public const string Light = "light_component";
      public const string Camera = "camera_component";
      public const string Animation = "animation_controller";
    }
  }
}