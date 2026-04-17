namespace Other
{
  public class RenderingComponent : Component
  {
    public Mesh Mesh { get; set; }
    public Material Material { get; set; }

    public RenderingComponent(ulong object_id)
      : base(object_id)
    {
    }
  }
}