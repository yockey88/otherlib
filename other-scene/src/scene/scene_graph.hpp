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
    scene_graph(scene_graph&& other) {
      g = std::move(other.g);
      id_pairs = std::move(other.id_pairs);
    }
    scene_graph& operator=(scene_graph&& other) {
      if (this != &other) {
        g = std::move(other.g);
        id_pairs = std::move(other.id_pairs);
      }
      return *this;
    }
    scene_graph(const scene_graph& other) = delete;
    scene_graph& operator=(const scene_graph& other) = delete;
    ~scene_graph();

    /// SCENE id (hash of file name)
    bool has_scene(natural_t id) const;
    bool has_scene(const std::string_view name) const;

    std::pair<uint64_t, scene*> create_new_scene(const std::string_view name);
    std::pair<uint64_t, scene*> load_scene(const filepath& scene_path);
    /// SCENE id (hash of file name)
    void remove_scene(uint64_t id);

    natural_t get_id_of_scene(const std::string_view name) const;

    scene* find_scene(const filepath& scene_path);
    scene* find_scene(const std::string& name);
    /// SCENE id (hash of file name)
    scene* find_scene(natural_t id);

    template <typename Fn>
      requires std::invocable<Fn, const scene&> && std::same_as<std::invoke_result_t<Fn, const scene&>, bool>
    scene* find_scene(Fn fn) {
      return g.find_item([&fn](const scene& s) { return fn(s); });
    }

    void clear();

    auto begin() { return g.begin(); }
    auto end() { return g.end(); }

   private:
    // (name or NODE id)
    scene* get_scene(const std::string_view id);
    scene* get_scene(uint64_t id);

    struct id_pair {
      natural_t node_id;
      natural_t scene_hash;
    };
    graph<scene> g;

    std::vector<id_pair> id_pairs;
  };

}  // namespace other

#endif  // OTHER_SCENE_SCENE_SCENE_GRAPH_HPP
