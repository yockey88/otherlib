using System.Reflection.Metadata;

namespace Other
{
  public struct Vertex
  {
    public static readonly int Stride = 22;
    
    public Vec3 Position { get; set; }
    public Vec3 Normal { get; set; }
    public Vec3 Tangent { get; set; }
    public Vec3 Bitangent { get; set; }
    public Vec2 TexCoord { get; set; }
    public IntVec4 BoneIds { get; set; }
    public Vec4 BoneWeights { get; set; }

    public Vertex()
    {
      Position = new Vec3(0f, 0f, 0f);
      Normal = new Vec3(0f, 0f, 1f);
      Tangent = new Vec3(1f, 0f, 0f);
      Bitangent = new Vec3(0f, 1f, 0f);
      TexCoord = new Vec2(0f, 0f);
      BoneIds = new IntVec4(0, 0, 0, 0);
      BoneWeights = new Vec4(0f, 0f, 0f, 0f);  
    }
    public Vertex(Vec3 position)
      : this()
    {
      Position = position;
    }
    public Vertex(Vec3 position, Vec3 normal)
      : this(position)
    {
      Normal = normal;
    }
    public Vertex(Vec3 position, Vec3 normal, Vec3 tangent, Vec3 bitangent)
      : this(position, normal)
    {
      Tangent = tangent;
      Bitangent = bitangent;
    }
    public Vertex(Vec3 position, Vec3 normal, Vec3 tangent, Vec3 bitangent, Vec2 tex_coords)
      : this(position, normal, tangent, bitangent)
    {
      TexCoord = tex_coords;
    }
    public Vertex(Vec3 position, Vec3 normal, Vec3 tangent, Vec3 bitangent, Vec2 tex_coords, IntVec4 bone_ids, Vec4 bone_weights)
      : this(position, normal, tangent, bitangent, tex_coords)
    {
      BoneIds = bone_ids;
      BoneWeights = bone_weights;
    }

    public override string ToString()
    {
      return $"Vertex(Position: {Position}, Normal: {Normal}, Tangent: {Tangent}, Bitangent: {Bitangent}, TexCoord: {TexCoord}, BoneIds: {BoneIds}, BoneWeights: {BoneWeights})";
    }

    internal RawVertex ToRawVertex()
    {
      return new RawVertex(Position, Normal, Tangent, Bitangent, TexCoord, BoneIds, BoneWeights);
    }
  }
}