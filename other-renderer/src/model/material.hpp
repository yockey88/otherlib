/**
 * \file model/material.hpp
 **/
#ifndef OTHER_RENDERER_MODEL_MATERIAL_HPP
#define OTHER_RENDERER_MODEL_MATERIAL_HPP

#include <glm/glm.hpp>

#include "core/defines.hpp"

namespace other {

  struct material {
    glm::vec3 ambient_color = glm::vec3(1.0f);
    glm::vec3 diffuse_color = glm::vec3(1.0f);
    glm::vec3 specular_color = glm::vec3(1.0f);

    float roughness = 0.5f;
    float metalness = 0.0f;
  };

}  // namespace other

#endif  // OTHER_RENDERER_MODEL_MATERIAL_HPP