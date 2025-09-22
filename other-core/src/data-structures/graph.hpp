/**
 * \file data-structures/graph.hpp
 **/
#ifndef OTHER_CORE_DATA_STRUCTURES_GRAPH_HPP
#define OTHER_CORE_DATA_STRUCTURES_GRAPH_HPP

#include "math/dynamic_matrix.hpp"

#include "data-structures/arena_vector.hpp"

namespace other {

  template <typename T>
  class graph {
   public:
    graph() = default;
    ~graph() = default;

    uint64_t add_node(T&& value) {
      return nodes.emplace_back(node{ get_next_id(), std::move(value) }).id;
    }

    void remove_node(uint64_t id) {
      auto itr = nodes.find_if([id](const auto& n) { return n.id == id; });
      if (itr != nodes.end()) {
        nodes.erase(itr);
      }
    }

    T* ptr_to_node_value(uint64_t id) {
      auto itr = nodes.find_if([id](const auto& n) { return n.id == id; });
      if (itr != nodes.end()) {
        return &itr.ptr->value;
      }
      return nullptr;
    }

    std::vector<uint64_t> get_all_node_ids() const {
      std::vector<uint64_t> ids;
      for (natural_t i = 0; i < nodes.size; ++i) {
        ids.push_back(nodes[i].id);
      }
      return ids;
    }

    // std::vector<uint64_t> get_node_neighbors(uint64_t id) const {
    //   std::vector<uint64_t> neighbors;
    //   auto itr = nodes.find_if([id](const auto& n) { return n.id == id; });
    //   if (itr != nodes.end()) {
    //     natural_t index = itr.ptr - nodes.data;
    //     for (natural_t j = 0; j < adjacency_matrix.cols; ++j) {
    //       if (adjacency_matrix(index, j) != 0) {
    //         neighbors.push_back(nodes[j].id);
    //       }
    //     }
    //   }
    //   return neighbors;
    // }

   private:
    struct node {
      uint64_t id = 0;
      T value;

      constexpr auto operator<=>(const node& other) const = default;
    };
    arena_vector<node> nodes = {};

    uint64_t id_counter = 0;
    uint64_t get_next_id() {
      return ++id_counter;
    }
  };

}  // namespace other

#endif  // OTHER_CORE_DATA_STRUCTURES_GRAPH_HPP