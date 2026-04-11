using System;
using System.Collections.Generic;

namespace Other
{
  public class Mesh
  {
    private List<float> vertices;
    private List<int> indices;
    // private List<Triangle> triangles;

    private int vertex_count;
    private int triangle_count;
    private int index_count;

    public List<float> Vertices
    {
      get { return vertices; }
      set { vertices = value; }
    }

    public List<int> Indices
    {
      get { return indices; }
      set { indices = value; }
    }

    public Mesh()
    {
      vertices = new List<float>();
      indices = new List<int>();
      vertex_count = 0;
      triangle_count = 0;
      index_count = 0;
    }

    public void AddVertex(float x, float y, float z)
    {
      vertices.Add(x);
      vertices.Add(y);
      vertices.Add(z);
      vertex_count++;
    }

    public void AddIndex(int index)
    {
      indices.Add(index);
      index_count++;

      if (index_count % 3 == 0)
      {
        triangle_count++;
      }
    }
  }
}