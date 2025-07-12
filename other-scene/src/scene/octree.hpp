/**
 * \file scene/octree.hpp
 **/
#ifndef OTHER_SCENE_SCENE_OCTREE_HPP
#define OTHER_SCENE_SCENE_OCTREE_HPP

#include <glm/glm.hpp>

#include "core/defines.hpp"

namespace other {

  struct octant {
    size_t index;
    glm::vec3 min;
    glm::vec3 max;

    octant* children[8] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
  };

  class octree {
   public:
    octree(glm::vec3 extent, natural_t resolution);
    ~octree();

   private:
    octant* root = nullptr;

    /// \note 0 implies root
    void initialize_octant(octant* sector, octant* parent = nullptr);
  };

}  // namespace other

#endif  // OTHER_SCENE_SCENE_OCTREE_HPP