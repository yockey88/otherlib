/**
 * \file model/model_data.hpp
 **/
#ifndef OTHER_RENDERER_MODEL_MODEL_DATA_HPP
#define OTHER_RENDERER_MODEL_MODEL_DATA_HPP

#include <glm/glm.hpp>

#include "model/animation.hpp"
#include "model/material.hpp"
#include "model/skeleton.hpp"
#include "model/vertex.hpp"

namespace other {

  struct imported_material {
    std::string name;
    glm::vec4 base_color = glm::vec4(1.f);  // AI_MATKEY_COLOR_DIFFUSE / gltf baseColorFactor
    glm::vec3 emissive_color = glm::vec3(0.f);

    float roughness = 1.f;
    float metalness = 0.f;

    std::string base_color_texture;  // empty = none; gltf uri / fbx relative path
    std::string normal_texture;
    std::string metallic_roughness_texture;
    std::string emissive_texture;
  };

  struct model_data {
    std::string name;
    glm::mat4 global_transform = glm::mat4(1.f);
    glm::mat4 inverse_global_transform = glm::mat4(1.f);

    ostd::vector<vertex> vertices;  // vertex slimmed: id/connected_triangles deleted (§2)
    ostd::vector<index> indices;
    ostd::vector<submesh> submeshes;
    ostd::vector<mesh_node> nodes;

    ostd::vector<imported_material> materials;

    skeleton skel;
    // ostd::vector<animation_clip> clips;

    bounding_box bounds = bounding_box::empty;

    bool valid() const {  // texture_importer::texture_data::valid() pattern
      return !vertices.empty() && !indices.empty() && !submeshes.empty() && !nodes.empty();
    }
  };

}  // namespace other

#endif  // OTHER_RENDERER_MODEL_MODEL_DATA_HPP