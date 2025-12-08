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
    void remove_scene(uint64_t id);

    natural_t get_id_of_scene(const std::string_view name) const;

    scene* get_scene(uint64_t id);

   private:
    graph<scene> g = {};
  };

}  // namespace other

#endif  // OTHER_SCENE_SCENE_SCENE_GRAPH_HPP
