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

  bool scene_graph::has_scene(natural_t id) const {
    return g.find_item([&id](const scene& s) {
      return s.id == id;
    }) != nullptr;
  }

  bool scene_graph::has_scene(const std::string_view name) const {
    return g.find_item([&name](const scene& s) {
      return s.name == name;
    }) != nullptr;
  }

  std::pair<natural_t, scene*> scene_graph::create_new_scene(const std::string_view name) {
    natural_t id = g.add_node(scene(name));
    return { id, g.ptr_to_node_value(id) };
  }

  std::pair<uint64_t, scene*> scene_graph::load_scene(const filepath& scene_path) {
    scene new_scene = scene::load_scene(scene_path);
    if (new_scene.id == 0) {
      return { 0, nullptr };
    }

    natural_t id = g.add_node(std::move(new_scene));
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