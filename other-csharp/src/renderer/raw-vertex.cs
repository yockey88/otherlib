using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Other
{
  [StructLayout(LayoutKind.Sequential)]
  public struct RawVertex
  {
    public float pos_x;
    public float pos_y;
    public float pos_z;

    public float normal_x;
    public float normal_y;
    public float normal_z;

    public float tangent_x;
    public float tangent_y;
    public float tangent_z;

    public float bitangent_x;
    public float bitangent_y;
    public float bitangent_z;

    public float tex_coord_x;
    public float tex_coord_y;

    public int bone_id_1;
    public int bone_id_2;
    public int bone_id_3;
    public int bone_id_4;

    public float bone_weight_1;
    public float bone_weight_2;
    public float bone_weight_3;
    public float bone_weight_4;

    private void SetData(float x, float y, float z, float nx, float ny, float nz, float tx, float ty, float tz, float btx, float bty, float btz, float u, float v, 
                         int b1, int b2, int b3, int b4, float bw1, float bw2, float bw3, float bw4)
    {
      pos_x = x;
      pos_y = y;
      pos_z = z;

      normal_x = nx;
      normal_y = ny;
      normal_z = nz;

      tangent_x = tx;
      tangent_y = ty;
      tangent_z = tz;

      bitangent_x = btx;
      bitangent_y = bty;
      bitangent_z = btz;

      tex_coord_x = u;
      tex_coord_y = v;

      bone_id_1 = b1;
      bone_id_2 = b2; 
      bone_id_3 = b3;
      bone_id_4 = b4;

      bone_weight_1 = bw1;
      bone_weight_2 = bw2;
      bone_weight_3 = bw3;
      bone_weight_4 = bw4;
    }

    public RawVertex(Vec3 pos)
    {
      SetData(pos.X, pos.Y, pos.Z, 0f, 0f, 1f, 1f, 0f, 0f, 0f, 1f, 0f, 0f, 0f, 0, 0, 0, 0, 0f, 0f, 0f, 0f);
    }
    public RawVertex(Vec3 pos, Vec3 normal)
    {
      SetData(pos.X, pos.Y, pos.Z, normal.X, normal.Y, normal.Z, 1f, 0f, 0f, 0f, 1f, 0f, 0f, 0f, 0, 0, 0, 0, 0f, 0f, 0f, 0f);
    }
    public RawVertex(Vec3 pos, Vec3 normal, Vec3 tangent, Vec3 bitangent)
    {
      SetData(pos.X, pos.Y, pos.Z, normal.X, normal.Y, normal.Z, tangent.X, tangent.Y, tangent.Z, bitangent.X, bitangent.Y, bitangent.Z, 0f, 0f, 0, 0, 0, 0, 0f, 0f, 0f, 0f);
    }
    public RawVertex(Vec3 pos, Vec3 normal, Vec3 tangent, Vec3 bitangent, Vec2 tex_coords)
    {
      SetData(pos.X, pos.Y, pos.Z, normal.X, normal.Y, normal.Z, tangent.X, tangent.Y, tangent.Z, bitangent.X, bitangent.Y, bitangent.Z, tex_coords.X, tex_coords.Y, 0, 0, 0, 0, 0f, 0f, 0f, 0f);
    }
    public RawVertex(Vec3 pos, Vec3 normal, Vec3 tangent, Vec3 bitangent, Vec2 tex_coords, IntVec4 bone_ids, Vec4 bone_weights)
    {
      SetData(pos.X, pos.Y, pos.Z, normal.X, normal.Y, normal.Z, tangent.X, tangent.Y, tangent.Z, bitangent.X, bitangent.Y, bitangent.Z, tex_coords.X, tex_coords.Y,
              bone_ids.X, bone_ids.Y, bone_ids.Z, bone_ids.W, bone_weights.X, bone_weights.Y, bone_weights.Z, bone_weights.W);
    }

    public void AddToList(List<float> vertexList)
    {
      vertexList.Add(pos_x);
      vertexList.Add(pos_y);
      vertexList.Add(pos_z);

      vertexList.Add(normal_x);
      vertexList.Add(normal_y);
      vertexList.Add(normal_z);

      vertexList.Add(tangent_x);
      vertexList.Add(tangent_y);
      vertexList.Add(tangent_z);

      vertexList.Add(bitangent_x);
      vertexList.Add(bitangent_y);
      vertexList.Add(bitangent_z);

      vertexList.Add(tex_coord_x);
      vertexList.Add(tex_coord_y);

      vertexList.Add(bone_id_1);
      vertexList.Add(bone_id_2);
      vertexList.Add(bone_id_3);
      vertexList.Add(bone_id_4);

      vertexList.Add(bone_weight_1);
      vertexList.Add(bone_weight_2);
      vertexList.Add(bone_weight_3);
      vertexList.Add(bone_weight_4);
    }
  }
}