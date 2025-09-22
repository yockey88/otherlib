/**
 * \file scene/scene_graph.cpp
 **/
#include "scene/scene_graph.hpp"

namespace other {

  scene_graph::~scene_graph() {
  }

  std::pair<uint64_t, scene*> scene_graph::create_new_scene(const std::string& name) {
    uint64_t id = g.add_node(scene::create_scene(name));
    return { id, g.ptr_to_node_value(id) };
  }

  void scene_graph::remove_scene(uint64_t id) {
    g.remove_node(id);
  }

  scene* scene_graph::get_scene(uint64_t id) {
    return g.ptr_to_node_value(id);
  }

}  // namespace other