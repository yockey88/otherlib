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

    public override void Enable()
    {
      OnEnable();
    }
    public virtual void OnEnable() {}

    public override void Disable()
    {
      OnDisable();
    }
    public virtual void OnDisable() {}
    
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