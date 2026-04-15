using System;
using System.Collections.Generic;

namespace Other
{
  public struct Triangle 
  {
    public int Vertex1;
    public int Vertex2;
    public int Vertex3;

    public Triangle(int v1, int v2, int v3)
    {
      Vertex1 = v1;
      Vertex2 = v2;
      Vertex3 = v3;
    }
  }

  public class Mesh
  {
    private List<float> vertices;
    private List<int> indices;
    private List<Triangle> triangles;
    private int vertex_count;
    

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

    public List<Triangle> Triangles
    {
      get { return triangles; }
      set { triangles = value; }
    }

    public int VertexCount
    {
      get { return vertex_count; }
    }
    public int TriangleCount
    {
      get { return triangles.Count; }
    }


    public Mesh()
    {
      vertices = new List<float>();
      indices = new List<int>();
      triangles = new List<Triangle>();
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
      if (indices.Count % 3 == 0)
      {
        AddTriangle();
      }
    }

    private void AddTriangle()
    {
      int v1 = indices[indices.Count - 3];
      int v2 = indices[indices.Count - 2];
      int v3 = indices[indices.Count - 1];
      triangles.Add(new Triangle(v1, v2, v3));
    }
  }
}