namespace Other
{
  /// simply defers all functions to user's implementations
  public class SceneBehavior : Core.OtherBehavior
  {
    public SceneObject SceneObject
    {
      get
      {
        if (ParentObject is SceneObject scene_obj)
        {
          return scene_obj;
        }
        throw new System.Exception("SceneBehavior must be attached to a SceneObject.");
      }
    }

    public SceneObjectHandle Handle
    {
      get
      {
        return SceneObject!.ObjectHandle;
      }
    }
    
    public Transform Transform
    {
      get
      {
        return Handle?.Transform;
      }
    }

    protected T GetComponent<T>() where T : Component => SceneObject.GetComponent<T>();
    protected bool HasComponent<T>() where T : Component => SceneObject.HasComponent<T>();
    protected void AddComponent<T>() where T : Component => SceneObject.AddComponent<T>();

    protected override void Awake()
    {
      OnAwake();
    }
    protected virtual void OnAwake() {}

    protected override void Remove()
    {
      OnRemove();
    }
    protected virtual void OnRemove() {}

    protected override void Enable()
    {
      OnEnable();
    }
    protected virtual void OnEnable() {}

    protected override void Disable()
    {
      OnDisable();
    }
    protected virtual void OnDisable() {}

    protected override void Update()
    {
      OnUpdate();
    }
    protected virtual void OnUpdate() {}

    protected override void LateUpdate()
    {
      OnLateUpdate();
    }
    protected virtual void  OnLateUpdate() {}

    protected override void FixedUpdate()
    {
      OnFixedUpdate();
    }
    protected virtual void OnFixedUpdate() {}

    protected override void Render()
    {
      OnRender();
    }
    /// draw hook called every frame, playing or not; submit Draw calls here so
    /// debug/scene overlays stay visible while editing
    protected virtual void OnRender() {}
  }
}