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
        return null;
      }
    }

    public SceneObjectHandle Handle
    {
      get
      {
        return SceneObject?.ObjectHandle;
      }
    }

    public Transform Transform
    {
      get
      {
        return Handle?.Transform;
      }
    }

    protected T GetComponent<T>() 
      where T : Component
    {
      if (SceneObject == null)
      {
        return null;
      }
      return SceneObject.GetComponent<T>();
    }

    protected bool HasComponent<T>() 
      where T : Component
    {
      if (SceneObject == null)
      {
        return false;
      }
      return SceneObject.HasComponent<T>();
    }

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
  }
}