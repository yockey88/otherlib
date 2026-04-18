using System;
using Other;

class TestBehavior : SceneBehavior
{
  protected override void OnAwake()
  {
    Mesh new_mesh = new Mesh("cube");
    new_mesh.AddVertex(new Vec3(-1f, -1f, -1f), new Vec3(-1f, -1f, -1f));
    new_mesh.AddVertex(new Vec3(1f, -1f, -1f), new Vec3(1f, -1f, -1f));
    new_mesh.AddVertex(new Vec3(1f, 1f, -1f), new Vec3(1f, 1f, -1f));
    new_mesh.AddVertex(new Vec3(-1f, 1f, -1f), new Vec3(-1f, 1f, -1f));
    new_mesh.AddVertex(new Vec3(-1f, -1f, 1f), new Vec3(-1f, -1f, 1f));
    new_mesh.AddVertex(new Vec3(1f, -1f, 1f), new Vec3(1f, -1f, 1f));
    new_mesh.AddVertex(new Vec3(1f, 1f, 1f), new Vec3(1f, 1f, 1f));
    new_mesh.AddVertex(new Vec3(-1f, 1f, 1f), new Vec3(-1f, 1f, 1f));

    new_mesh.AddIndices(0, 1, 2);
    new_mesh.AddIndices(0, 2, 3);
    new_mesh.AddIndices(4, 5, 6);
    new_mesh.AddIndices(4, 6, 7);
    new_mesh.AddIndices(0, 1, 5);
    new_mesh.AddIndices(0, 5, 4);
    new_mesh.AddIndices(2, 3, 7);
    new_mesh.AddIndices(2, 7, 6);
    new_mesh.AddIndices(1, 2, 6);
    new_mesh.AddIndices(1, 6, 5);
    new_mesh.AddIndices(0, 3, 7);
    new_mesh.AddIndices(0, 7, 4);

    Material new_mat = new Material("cube_mat")
    {
      DiffuseColor = new Vec3(0.4f, 0.6f, 0.8f),
      DiffuseReflectivity = 0.5f,
      SpecularColor = new Vec3(0.8f, 0.8f, 0.8f),
      SpecularReflectivity = 0.5f,
      Emissivity = 0.1f,
      Shininess = 16.0f,
      Transparency = 0.0f
    };

    if (!HasComponent<RenderComponent>())
    {
      AddComponent<RenderComponent>();
    }

    RenderComponent render_comp = GetComponent<RenderComponent>();
    render_comp.Model = new_mesh;
    render_comp.Material = new_mat;
  }

  protected override void OnRemove()
  {
    Debug.Warn("TestBehavior: OnRemove called.");
  }

  protected override void OnEnable()
  {
    Debug.Warn("TestBehavior: OnEnable called.");
  }

  protected override void OnDisable()
  {
    Debug.Warn("TestBehavior: OnDisable called.");
  }

  protected override void OnUpdate()
  {
  }
}