/**
 * \file scene/scene_graph.hpp
 **/
#ifndef OTHER_SCENE_SCENE_SCENE_GRAPH_HPP
#define OTHER_SCENE_SCENE_SCENE_GRAPH_HPP

#include "scene/scene.hpp"

#include "data-structures/graph.hpp"

namespace other {

  class scene;

  class scene_graph {
   public:
    scene_graph(std::vector<scene>& scenes);
    ~scene_graph();

    std::pair<uint64_t, scene*> create_new_scene(const std::string& name);
    void remove_scene(uint64_t id);

    scene* get_scene(uint64_t id);

   private:
    graph<scene> g = {};
  };

}  // namespace other

#endif  // OTHER_SCENE_SCENE_SCENE_GRAPH_HPP
