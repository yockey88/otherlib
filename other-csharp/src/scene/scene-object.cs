using System;
using System.Collections.Generic;
using OtherCsBindings;

namespace Other
{
  public class SceneObject : Core.OtherObject
  {
    public readonly List<Core.Behavior> behaviors = new();
    public SceneObject(IntPtr native_handle)
      : base(native_handle)
    {
    }
  }
}