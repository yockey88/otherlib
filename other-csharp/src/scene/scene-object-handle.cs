using System;
using OtherCsBindings;

namespace Other
{
  public class SceneObjectHandle : IEquatable<SceneObjectHandle>
  {
    [NativeFunction("ValidateObjectHandle")]
    internal static unsafe delegate*<ulong, uint, uint, NativeBool32> NativeValidate;


    private readonly ulong id;
    private readonly uint generation;
    private readonly uint scene_id;
    public SceneObjectHandle(ulong object_id, uint generation = 0, uint scene_id = 0)
    {
      this.id = object_id;
      this.generation = generation;
      this.scene_id = scene_id;
    }

    private Transform cached_transform;


    public ulong Id => id;
    public uint Generation => generation;
    public uint SceneId => scene_id;
    public bool IsValid => Scene.HasObject(id);
    public bool Validate()
    {
      unsafe { return NativeValidate(id, generation, scene_id); }
    }
    public string Name
    {
      get => Scene.GetObjectName(id);
      set => Scene.SetObjectName(id, value);
    }
    public bool Visible
    {
      get => Scene.GetObjectVisible(id);
      set => Scene.SetObjectVisible(id, value);
    }

    public Transform Transform
    {
      get
      {
        cached_transform ??= new Transform(id);
        return cached_transform;
      }
    }

    public SceneObjectHandle Parent
    {
      get
      {
        ulong parent_id = Scene.GetParentId(id);
        return parent_id != 0 ? new SceneObjectHandle(parent_id) : null;
      }
    }

    public SceneObjectHandle[] Children
    {
      get
      {
        ulong[] child_ids = Scene.GetChildrenIds(id);
        SceneObjectHandle[] result = new SceneObjectHandle[child_ids.Length];
        for (int i = 0; i < child_ids.Length; i++)
          result[i] = new SceneObjectHandle(child_ids[i]);
        return result;
      }
    }

    public bool HasTag(string tag) => Scene.ObjectHasTag(id, tag);
    public void AddTag(string tag) => Scene.AddObjectTag(id, tag);
    public void RemoveTag(string tag) => Scene.RemoveObjectTag(id, tag);

    public void Destroy()
    {
      Scene.DestroyObject(id);
    }

    public static SceneObjectHandle Create(string name, Vec3 position = default)
    {
      ulong id = Scene.CreateObject(name, position);
      return new SceneObjectHandle(id);
    }

    public static SceneObjectHandle Find(string name)
    {
      ulong id = Scene.FindObjectByName(name);
      return id != 0 ? new SceneObjectHandle(id) : null;
    }

    public bool Equals(SceneObjectHandle other) =>
      other != null && 
      id == other.id && 
      generation == other.generation && 
      scene_id == other.scene_id;
    public override bool Equals(object obj) => obj is SceneObjectHandle other && Equals(other);
    public override int GetHashCode() => id.GetHashCode();
    public override string ToString() => $"SceneObject({id}, \"{Name}\")";

    public static bool operator==(SceneObjectHandle left, SceneObjectHandle right) => left.Equals(right);
    public static bool operator!=(SceneObjectHandle left, SceneObjectHandle right) => !left.Equals(right);
  }
}