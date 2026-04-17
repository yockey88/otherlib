using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Other
{
  [StructLayout(LayoutKind.Sequential)]
  public struct RawIndex
  {
    int vertex_index_1;
    int vertex_index_2;
    int vertex_index_3;

    public RawIndex(int v1, int v2, int v3)
    {
      vertex_index_1 = v1;
      vertex_index_2 = v2;
      vertex_index_3 = v3;
    }

    public void AddToList(List<int> indexList)
    {
      indexList.Add(vertex_index_1);
      indexList.Add(vertex_index_2);
      indexList.Add(vertex_index_3);
    }
  }
}