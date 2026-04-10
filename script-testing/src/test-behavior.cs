using System;
using Other;

class TestBehavior : SceneBehavior
{
  protected override void OnAwake()
  {
    Mesh new_mesh = new Mesh();
  }

  protected override void OnRemove()
  {
    Debug.Warn("TestBehavior Remove");
  }

  protected override void OnUpdate()
  {
    Debug.Warn("TestBehavior Update");
  }
}