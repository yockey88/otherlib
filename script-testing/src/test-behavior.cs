using System;
using Other;

class TestBehavior : SceneBehavior
{
  protected override void OnAwake()
  {
    // Mesh new_mesh = new Mesh();
    // /// make a cube
    // new_mesh.AddVertex(-0.5f, -0.5f, -0.5f);
    // new_mesh.AddVertex(0.5f, -0.5f, -0.5f);
    // new_mesh.AddVertex(0.5f, 0.5f, -0.5f);
    // new_mesh.AddVertex(-0.5f, 0.5f, -0.5f);
    // new_mesh.AddVertex(-0.5f, -0.5f, 0.5f);
    // new_mesh.AddVertex(0.5f, -0.5f, 0.5f);
    // new_mesh.AddVertex(0.5f, 0.5f, 0.5f);
    // new_mesh.AddVertex(-0.5f, 0.5f, 0.5f);

    // new_mesh.AddIndex(0); new_mesh.AddIndex(1); new_mesh.AddIndex(2);
    // new_mesh.AddIndex(0); new_mesh.AddIndex(2); new_mesh.AddIndex(3);
    // new_mesh.AddIndex(4); new_mesh.AddIndex(5); new_mesh.AddIndex(6);
    // new_mesh.AddIndex(4); new_mesh.AddIndex(6); new_mesh.AddIndex(7);
    // new_mesh.AddIndex(0); new_mesh.AddIndex(1); new_mesh.AddIndex(5);
    // new_mesh.AddIndex(0); new_mesh.AddIndex(5); new_mesh.AddIndex(4);
    // new_mesh.AddIndex(2); new_mesh.AddIndex(3); new_mesh.AddIndex(7);
    // new_mesh.AddIndex(2); new_mesh.AddIndex(7); new_mesh.AddIndex(6);
    // new_mesh.AddIndex(1); new_mesh.AddIndex(2); new_mesh.AddIndex(6);
    // new_mesh.AddIndex(1); new_mesh.AddIndex(6); new_mesh.AddIndex(5);
    // new_mesh.AddIndex(3); new_mesh.AddIndex(0); new_mesh.AddIndex(4);
    // new_mesh.AddIndex(3); new_mesh.AddIndex(4); new_mesh.AddIndex(7);

    // // GetComponent<RendererComponent>()?.SetMesh(new_mesh);
    // Debug.Warn("TestBehavior: OnAwake called, created a cube mesh but did not set it to any renderer component.");
  }

  protected override void OnRemove()
  {
    Debug.Warn("TestBehavior: OnRemove called.");
  }

  protected override void OnEnable()
  {
    Debug.Warn("TestBehavior: OnEnable called.");
    GetComponent<Transform>().Position = new Vec3(0, 0.01f, 0);
  }

  protected override void OnDisable()
  {
    Debug.Warn("TestBehavior: OnDisable called.");
  }

  protected override void OnUpdate()
  {
  }
}