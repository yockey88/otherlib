namespace Other
{
  public class RenderingComponent
  {
    ulong object_id;

    public Mesh Mesh { get; set; }
    public Material Material { get; set; }

    public RenderingComponent(ulong object_id)
    {
      this.object_id = object_id;  
    }
  }
}