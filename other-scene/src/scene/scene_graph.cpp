/**
 * \file scene/scene_graph.cpp
 **/
#include "scene/scene_graph.hpp"

namespace other {

  scene_graph::scene_graph(std::vector<scene>& scenes) {
    for (auto& s : scenes) {
      g.add_node(std::move(s));
    }
  }

  scene_graph::~scene_graph() {
    g.clear();
  }

  std::pair<natural_t, scene*> scene_graph::create_new_scene(const std::string_view name) {
    natural_t id = g.add_node(scene(name));
    return { id, g.ptr_to_node_value(id) };
  }

  void scene_graph::remove_scene(natural_t id) {
    g.remove_node(id);
  }

  natural_t scene_graph::get_id_of_scene(const std::string_view name) const {
    const scene* found_scene = g.find_item([&name](const scene& s) {
      return s.name == name;
    });
    if (found_scene != nullptr) {
      return found_scene->id;
    }
    return 0;
  }

  scene* scene_graph::get_scene(natural_t id) {
    return g.ptr_to_node_value(id);
  }

}  // namespace other