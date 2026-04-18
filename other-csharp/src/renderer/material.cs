using System;
using System.Collections.Generic;
using OtherCsBindings;

namespace Other
{
  public class Material
  {
    private struct Color
    {
      public Vec3 color;
      public float reflectivity;
    }
    [NativeFunction("FetchMaterial")]
    internal static unsafe delegate*<UInt64, Vec3*, float*, Vec3*, float*, Vec3*, float*, float*, float*, void> NativeFetchMaterial;
    [NativeFunction("UploadMaterial")]
    internal static unsafe delegate*<UInt64, NativeString, Vec3*, float*, Vec3*, float*, Vec3*, float*, float*, float*, void> NativeUploadMaterial;

    private List<Texture> textures;
    private Color diffuse;
    private Color specular;
    private Color emissive;
    private float transparency;
    private float shininess;

    public string Name { get; set; }
    public Vec3 DiffuseColor 
    {
      get { return diffuse.color; }
      set { diffuse.color = value; }
    }
    public float DiffuseReflectivity
    {
      get { return diffuse.reflectivity; }
      set { diffuse.reflectivity = value; }
    }

    public Vec3 SpecularColor
    {
      get { return specular.color; }
      set { specular.color = value; }
    }
    public float SpecularReflectivity
    {
      get { return specular.reflectivity; }
      set { specular.reflectivity = value; }
    }

    public Vec3 EmissiveColor
    {
      get { return emissive.color; }
      set { emissive.color = value; }
    }
    public float Emissivity
    {
      get { return emissive.reflectivity; }
      set { emissive.reflectivity = value; }
    }

    public float Transparency
    {
      get { return transparency; }
      set { transparency = value; }
    }
    public float Shininess
    {
      get { return shininess; }
      set { shininess = value; }
    }

    public Material() 
      : this("default_material")
    {
    }
    public Material(string name)
    {
      Name = name;
      DiffuseColor = new Vec3(1, 1, 1);
      DiffuseReflectivity = 1.0f;
      SpecularColor = new Vec3(1, 1, 1);
      SpecularReflectivity = 1.0f;
      EmissiveColor = new Vec3(0, 0, 0);
      Emissivity = 0.0f;
      Transparency = 0.0f;
      Shininess = 32.0f;
      textures = new List<Texture>();
    }

    public void AddTexture(Texture texture)
    {
      textures.Add(texture);
    }

    public static Material GetDefault()
    {
      return new Material("default_material")
      {
        DiffuseColor = new Vec3(1, 1, 1),
        DiffuseReflectivity = 1.0f,
        SpecularColor = new Vec3(1, 1, 1),
        SpecularReflectivity = 1.0f,
        EmissiveColor = new Vec3(0, 0, 0),
        Emissivity = 0.0f,
        Transparency = 0.0f,
        Shininess = 32.0f,
      };
    }

    public static void FetchMaterial(ulong object_id, out Material? material)
    {
      material = null;
      unsafe
      {
        NativeString mat_name = RenderComponent.NativeGetMaterialName(object_id);
        if (mat_name.ToString() == null)
        {
          material = GetDefault();
          return;
        }

        material = new Material(mat_name.ToString()!);
        fixed(Vec3* diffuse_color_ptr = &material.diffuse.color)
        fixed(float* diffuse_reflectivity_ptr = &material.diffuse.reflectivity)
        fixed(Vec3* specular_color_ptr = &material.specular.color)
        fixed(float* specular_reflectivity_ptr = &material.specular.reflectivity)
        fixed(Vec3* emissive_color_ptr = &material.emissive.color)
        fixed(float* emissivity_ptr = &material.emissive.reflectivity)
        fixed(float* transparency_ptr = &material.transparency)
        fixed(float* shininess_ptr = &material.shininess)
        {
          NativeFetchMaterial(object_id, diffuse_color_ptr, diffuse_reflectivity_ptr, specular_color_ptr, specular_reflectivity_ptr, emissive_color_ptr, emissivity_ptr, transparency_ptr, shininess_ptr);
        }
      }
    }

    
    public static void UploadMaterial(ulong object_id, Material material)
    {
      if (material == null)
      {
        Logger.LogWarning("Attempted to upload null material, skipping.");
        return;
      }

      NativeString mat_name = new NativeString(material.Name);
      unsafe
      {
        fixed(Vec3* diffuse_color_ptr = &material.diffuse.color)
        fixed(float* diffuse_reflectivity_ptr = &material.diffuse.reflectivity)
        fixed(Vec3* specular_color_ptr = &material.specular.color)
        fixed(float* specular_reflectivity_ptr = &material.specular.reflectivity)
        fixed(Vec3* emissive_color_ptr = &material.emissive.color)
        fixed(float* emissivity_ptr = &material.emissive.reflectivity)
        fixed(float* transparency_ptr = &material.transparency)
        fixed(float* shininess_ptr = &material.shininess)
        {
          NativeUploadMaterial(object_id, mat_name, diffuse_color_ptr, diffuse_reflectivity_ptr, specular_color_ptr, specular_reflectivity_ptr, emissive_color_ptr, emissivity_ptr, transparency_ptr, shininess_ptr);
        }
      }
    }
  }
}