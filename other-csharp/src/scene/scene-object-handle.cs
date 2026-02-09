namespace Other
{
  public class SceneObjectHandle
  {
    private readonly ulong id;
    private Transform cached_transform;

    public SceneObjectHandle(ulong id)
    {
      this.id = id;
    }

    public ulong Id => id;
    public bool IsValid => Scene.HasObject(id);
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

    public bool HasComponent(string component_name) => Core.Components.Has(id, component_name);
    public void AddComponent(string component_name) => Core.Components.Add(id, component_name);
    public void RemoveComponent(string component_name) => Core.Components.Remove(id, component_name);

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

    public override bool Equals(object obj) => obj is SceneObjectHandle other && id == other.id;
    public override int GetHashCode() => id.GetHashCode();
    public override string ToString() => $"SceneObject({id}, \"{Name}\")";
  }
}