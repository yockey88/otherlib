using System;
using System.Collections.Generic;

namespace Other
{
  public class Mesh
  {
    private List<float> vertices;
    private List<int> indices;

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
    }

  }
}