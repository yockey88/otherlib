/**
 * \file scene/scene_graph.hpp
 **/
#ifndef OTHER_SCENE_SCENE_SCENE_GRAPH_HPP
#define OTHER_SCENE_SCENE_SCENE_GRAPH_HPP

#include "core/defines.hpp"

#include "scene/scene.hpp"

#include "data-structures/graph.hpp"

namespace other {

  class scene;

  class scene_graph {
   public:
    scene_graph() = default;
    scene_graph(std::vector<scene>& scenes);
    ~scene_graph();

    bool has_scene(natural_t id) const;
    bool has_scene(const std::string_view name) const;

    std::pair<uint64_t, scene*> create_new_scene(const std::string_view name);
    std::pair<uint64_t, scene*> load_scene(const filepath& scene_path);
    void remove_scene(uint64_t id);

    natural_t get_id_of_scene(const std::string_view name) const;

    scene* find_scene(const filepath& scene_path);
    scene* find_scene(const std::string& name);

    template <typename Fn>
      requires std::invocable<Fn, const scene&> && std::same_as<std::invoke_result_t<Fn, const scene&>, bool>
    scene* find_scene(Fn fn) {
      return g.find_item([&fn](const scene& s) { return fn(s); });
    }

    scene* get_scene(const std::string_view id);
    scene* get_scene(uint64_t id);

    auto begin() { return g.begin(); }
    auto end() { return g.end(); }

   private:
    graph<scene> g = {};
  };

}  // namespace other

#endif  // OTHER_SCENE_SCENE_SCENE_GRAPH_HPP
