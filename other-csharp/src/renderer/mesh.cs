using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Runtime.InteropServices;

namespace Other
{
  public class Mesh
  {
    private List<Vertex> vertices;
    private List<Index> indices;

    public string Name { get; set; }
    public List<Vertex> Vertices
    {
      get { return vertices; }
      set { vertices = value; }
    }
    public List<Index> Indices
    {
      get { return indices; }
      set { indices = value; }
    }
    public int VertexCount
    {
      get { return Vertices.Count; }
    }
    public int IndexCount
    {
      get { return indices.Count; }
    }

    public Mesh(string name)
    {
      Name = name;

      vertices = new List<Vertex>();
      indices = new List<Index>();
    }

    public void AddVertex(float x, float y, float z)
    {
      AddVertex(new Vec3(x, y, z));
    }
    public void AddVertex(Vec3 position)
    {
      vertices.Add(new Vertex(position));
    }
    public void AddVertex(Vec3 position, Vec3 normal)
    {
      vertices.Add(new Vertex(position, normal));
    }
    public void AddVertex(Vec3 position, Vec3 normal, Vec3 tangent, Vec3 bitangent)
    {
      vertices.Add(new Vertex(position, normal, tangent, bitangent));
    }
    public void AddVertex(Vec3 position, Vec3 normal, Vec3 tangent, Vec3 bitangent, Vec2 tex_coords)
    {
      vertices.Add(new Vertex(position, normal, tangent, bitangent, tex_coords));
    }
    public void AddVertex(Vec3 position, Vec3 normal, Vec3 tangent, Vec3 bitangent, Vec2 tex_coords, IntVec4 bone_ids, Vec4 bone_weights)
    {
      vertices.Add(new Vertex(position, normal, tangent, bitangent, tex_coords, bone_ids, bone_weights));
    }

    public void AddIndices(int i1, int i2, int i3)
    {
      indices.Add(new Index(i1, i2, i3));
    }
    public void AddIndex(Index index)
    {
      indices.Add(index);
    }
  }
}