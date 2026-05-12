using System;
using System.Runtime.InteropServices;

namespace Other
{
  public class Index
  {
    public int VertexIndex1 { get; set; }
    public int VertexIndex2 { get; set; }
    public int VertexIndex3 { get; set; }

    public Index(int v1, int v2, int v3)
    {
      VertexIndex1 = v1;
      VertexIndex2 = v2;
      VertexIndex3 = v3;
    }

    public override string ToString()
    {
      return $"Index({VertexIndex1}, {VertexIndex2}, {VertexIndex3})";
    }

    internal RawIndex ToRawIndex()
    {
      return new RawIndex(VertexIndex1, VertexIndex2, VertexIndex3);
    }
  } 
}