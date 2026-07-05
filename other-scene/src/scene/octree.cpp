/**
 * \file scene/octree.cpp
 **/
#include "scene/octree.hpp"

#include "memory/arena.hpp"

namespace other {

  octree::octree(glm::vec3 extent, natural_t resolution) {
    size_t num_sectors = resolution * resolution * resolution;
    size_t num_octants = num_sectors * 8;

    void* octant_mem = subsystem<arena>::get()->request_region(sizeof(octant) * num_octants, alignof(octant));
    root = new (octant_mem) octant[num_octants];

    /// rescursively initialize octants
    initialize_octant(root);
  }

  octree::~octree() {
    subsystem<arena>::get()->free_region(root);
    root = nullptr;
  }

  void octree::initialize_octant(octant* sector, octant* parent) {
  }

}  // namespace other