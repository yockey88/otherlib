using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using OtherCsBindings;

namespace Other
{
#nullable enable
  [NativeComponent("render_component")]
  public class RenderComponent : Component
  {
    [NativeFunction("GetNumVertices")]
    internal static unsafe delegate*<UInt64, int> NativeGetNumVertices;
    [NativeFunction("GetNumIndices")]
    internal static unsafe delegate*<UInt64, int> NativeGetNumIndices;
    [NativeFunction("GetMeshName")]
    internal static unsafe delegate*<UInt64, NativeString> NativeGetMeshName;
    [NativeFunction("GetMaterialName")]
    internal static unsafe delegate*<UInt64, NativeString> NativeGetMaterialName;
    [NativeFunction("GetActiveMaterialId")]
    internal static unsafe delegate*<UInt64, UInt64> NativeGetActiveMaterialId;
    [NativeFunction("FetchMesh")]
    internal static unsafe delegate*<UInt64, float*, int*, void> NativeFetchMesh;
    [NativeFunction("UploadMesh")]
    internal static unsafe delegate*<UInt64, NativeString, float*, int*, int*, int*, void> NativeUploadMesh;

    private Mesh? model = null;
    private Material? material = null;

    public RenderComponent(ulong object_id)
      : base(object_id)
    {
      model = null;
      material = null;
    }

    public Mesh? Model {
      get
      {
        if (model == null)
        {
          unsafe
          {
            model = null;
            NativeString mesh_name = NativeGetMeshName(ObjectId);
            if (mesh_name.ToString() != null)
            {
              model = new Mesh(mesh_name.ToString()!);
            }
            else
            {
              model = new Mesh("default_mesh");
            }
            
            float* vertices = stackalloc float[NativeGetNumVertices(ObjectId) * Vertex.Stride];
            int* indices = stackalloc int[NativeGetNumIndices(ObjectId)];

            NativeFetchMesh(ObjectId, vertices, indices);
            for (int i = 0; i < NativeGetNumVertices(ObjectId) * 3; i += 3)
            {
              model.AddVertex(
                new Vec3(vertices[i], vertices[i + 1], vertices[i + 2]),
                new Vec3(vertices[i + 3], vertices[i + 4], vertices[i + 5]),
                new Vec3(vertices[i + 6], vertices[i + 7], vertices[i + 8]),
                new Vec3(vertices[i + 9], vertices[i + 10], vertices[i + 11]),
                new Vec2(vertices[i + 12], vertices[i + 13]),
                new IntVec4((int)vertices[i + 14], (int)vertices[i + 15], (int)vertices[i + 16], (int)vertices[i + 17]),
                new Vec4(vertices[i + 18], vertices[i + 19], vertices[i + 20], vertices[i + 21])
              );
            }
            for (int i = 0; i < NativeGetNumIndices(ObjectId); i += 3)
            {
              model.AddIndices(indices[i], indices[i + 1], indices[i + 2]);
            }
          }
        }
        return model;
      }
      set
      {
        model = value;
        if (model == null)
        {
          unsafe
          {
            NativeString mat_name = NativeGetMaterialName(ObjectId);
            if (mat_name.ToString() != null)
            {
              model = new Mesh(mat_name.ToString()!);
            }
            else
            {
              model = new Mesh("default_material");
            }
          }
          return;
        }

        List<RawVertex> raw_vertices = new List<RawVertex>();
        List<float> vertex_data = new List<float>();
        model.Vertices.ForEach(vertex => raw_vertices.Add(vertex.ToRawVertex()));
        raw_vertices.ForEach(raw_vertex => raw_vertex.AddToList(vertex_data));
        
        List<RawIndex> raw_indices = new List<RawIndex>();
        List<int> index_data = new List<int>();
        model.Indices.ForEach(index => raw_indices.Add(index.ToRawIndex()));
        raw_indices.ForEach(raw_index => raw_index.AddToList(index_data));
        
        Span<float> vertices = CollectionsMarshal.AsSpan(vertex_data);
        Span<int> indices = CollectionsMarshal.AsSpan(index_data);
        unsafe
        {
          fixed (float* vertex_ptr = vertices)
          fixed (int* index_ptr = indices)
          {
            int vertex_count = model.VertexCount;
            int index_count = model.IndexCount;
            NativeUploadMesh(ObjectId, model.Name, vertex_ptr, &vertex_count, index_ptr, &index_count);
          }
        }
      }
    }

    public Material? Material {
      get
      {
        if (material == null)
        {
          Material.FetchMaterial(ObjectId, out material);
        }
        return material;
      }
      set
      {
        material = value;
        if (material == null)
        {
          unsafe
          {
            // ClearMaterial(ObjectId);
          }
        } 
        else
        {
          Material.UploadMaterial(ObjectId, material);
        }
      }
    }

    [NativeField("animated")]
    public bool Animated { get => GetBool(); set => SetBool(value); }
    [NativeField("visible")]
    public bool Visible { get => GetBool(); set => SetBool(value); }
    [NativeField("model_asset_id")]
    public UInt64 ModelAssetId { get => GetU64(); set => SetU64(value); }
  }
#nullable disable
}