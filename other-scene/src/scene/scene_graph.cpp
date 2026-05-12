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
    OTHER_ASSERT(std::filesystem::exists(scene_path), "Scene file '{}' does not exist.", scene_path.string());
    if (has_scene(scene_path.stem().string())) {
      CORE_LOG_WARN("Scene with name [{}] already exists in the scene graph. Cannot load duplicate scene.", scene_path.stem().string());
      const scene* existing_scene = g.find_item([&scene_path](const scene& s) { return s.name == scene_path.stem().string(); });
      OTHER_ASSERT(existing_scene != nullptr, "Scene with name [{}] not found in scene graph after confirming its existence.", scene_path.stem().string());
      return { existing_scene->id, g.ptr_to_node_value(existing_scene->id) };
    }

    auto [scene_id, scene_ptr] = create_new_scene(scene_path.stem().string());
    OTHER_ASSERT(scene_ptr != nullptr, "Failed to create new scene for loading scene file '{}'.", scene_path.string());
    scene_ptr->script_path = scene_path;
    return { scene_id, scene_ptr };
  }

  void scene_graph::remove_scene(natural_t id) {
    g.remove_node(id);
  }

  natural_t scene_graph::get_id_of_scene(const std::string_view name) const {
    const scene* found_scene = g.find_item([&name](const scene& s) { return s.name == name; });
    if (found_scene != nullptr) {
      CORE_LOG_INFO("Found scene [{}] with ID {} cached in scene graph.", name, found_scene->id);
      return found_scene->id;
    }
    CORE_LOG_WARN("Scene [{}] not found in scene graph.", name);
    return 0;
  }

  scene* scene_graph::find_scene(const filepath& scene_path) {
    std::string scene_name = scene_path.stem().string();
    return find_scene(scene_name);
  }

  scene* scene_graph::find_scene(const std::string& name) {
    natural_t id = get_id_of_scene(name);
    if (id == 0) {
      return nullptr;
    }
    return get_scene(id);
  }

  scene* scene_graph::get_scene(const std::string_view id) {
    natural_t scene_id = get_id_of_scene(id);
    if (scene_id == 0) {
      return nullptr;
    }
    return get_scene(scene_id);
  }

  scene* scene_graph::get_scene(natural_t id) {
    return g.ptr_to_node_value(id);
  }

  void scene_graph::clear() {
    g.clear();
  }

}  // namespace other