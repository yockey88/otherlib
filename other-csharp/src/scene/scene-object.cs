using System;
using OtherCsBindings;

namespace Other
{
  public class SceneObject : Core.OtherObject
  {
    [NativeFunction("GetWorldMatrix")]
    unsafe private static delegate*<UInt64, float*, void> NativeGetWorldMatrix;

    private SceneObjectHandle handle;
    public SceneObjectHandle ObjectHandle => handle;

    public SceneObject(IntPtr native_handle)
      : base(native_handle, ObjectType.SceneObject)
    {
      handle = new SceneObjectHandle(ObjectID);
    }

    public Mat4 WorldMatrix
    {
      get
      {
        unsafe
        {
          float* ptr = stackalloc float[16];
          NativeGetWorldMatrix(handle.Id, ptr);
          return new Mat4(
            ptr[0], ptr[1], ptr[2], ptr[3],
            ptr[4], ptr[5], ptr[6], ptr[7],
            ptr[8], ptr[9], ptr[10], ptr[11],
            ptr[12], ptr[13], ptr[14], ptr[15]
          );
        }
      }
    }

    public T AddBehavior<T>() 
      where T : Core.OtherBehavior, new()
    {
      T behavior = new T();
      AddBehavior(behavior);
      return behavior;
    }

    public T GetComponent<T>() 
      where T : Component
    {
      if (!HasComponent<T>())
      {
        throw new MissingComponentException($"SceneObject does not have component of type {typeof(T).Name}");
      }
      return Activator.CreateInstance(typeof(T), ObjectHandle.Id) as T;
    }

    public bool HasComponent<T>()
      where T : Component
    {
      bool res = false;
      unsafe 
      {
        res = OtherABI.NativeHasComponent(ObjectID, Component.TypeId<T>());
      }
      return res;
    }

    public void AddComponent<T>()
      where T : Component
    {
      unsafe
      {
        OtherABI.NativeAddComponent(ObjectID, Component.TypeId<T>());
      }
    }

    public void RemoveComponent<T>()
      where T : Component
    {
      unsafe
      {
        OtherABI.NativeRemoveComponent(ObjectID, Component.TypeId<T>());
      }
    }

    public override void OnStart()
    {
    }

    public override void OnStop()
    {
      
    }

    public override void OnUpdate()
    {
      
    }

    public override void OnLateUpdate()
    {
      
    }

    public override void OnFixedUpdate()
    {
      
    }
  }
}