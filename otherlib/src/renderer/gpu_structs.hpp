/**
 * \file renderer/gpu_structs.hpp
 **/
#ifndef OTHER_RENDERER_GPU_STRUCTS_HPP
#define OTHER_RENDERER_GPU_STRUCTS_HPP

#include <glm/glm.hpp>

#include "core/defines.hpp"

namespace other {
  namespace gpu {

    constexpr static inline size_t kGpuAlignment = 16;
#define GPU_ALIGN __declspec(align(kGpuAlignment))

    GPU_ALIGN struct sphere {
      glm::vec3 position;
      float radius;
    };

    constexpr size_t kMaxSpheres = 100;
    GPU_ALIGN struct sphere_buffer {
      sphere spheres[kMaxSpheres];
    };

    GPU_ALIGN struct material {
      glm::vec3 albedo;
      float reflectance;
      float absorption;
    };

    constexpr size_t kMaxMaterials = 2;
    GPU_ALIGN struct material_buffer {
      material materials[kMaxMaterials];
    };

    enum shape_type : int {
      SHAPE_SPHERE = 0,
      SHAPE_PLANE,
      SHAPE_TRIANGLE,
      SHAPE_CUBE,

      SHAPE_NUM_TYPES,
      INVALID_SHAPE = -1
    };

    GPU_ALIGN struct shape {
      shape_type type = INVALID_SHAPE;
      int shape_index = -1;
    };

    GPU_ALIGN struct object {
      shape sh;
      int material_index = -1;
    };

    constexpr size_t kMaxObjects = 100;
    GPU_ALIGN struct object_buffer {
      object objects[kMaxObjects];
    };

    GPU_ALIGN struct camera_data {
      glm::vec4 position;
      glm::vec4 forward;

      // near & far clip, padding x2
      glm::vec4 camera_features;

      glm::mat4 view_matrix;
      glm::mat4 projection_matrix;
    };

    GPU_ALIGN struct scene_metadata {
      glm::vec4 window_size;

      /// x = num_spheres, y = num_objects, z = num_materials, w = padding
      glm::ivec4 object_data;

      int samples_per_pixel = 100;
      int max_depth = 50;
    };

    GPU_ALIGN struct camera_buffer {
      glm::vec4 camera_position;
      glm::vec4 camera_forward;
    };

    GPU_ALIGN struct ray_gen_data {
      glm::vec4 pixel00_loc;
      glm::vec4 pixel_delta_u;
      glm::vec4 pixel_delta_v;
      glm::vec3 square_sample;
    };

  }  // namespace gpu
}  // namespace other

#endif  // OTHER_RENDERER_GPU_STRUCTS_HPP