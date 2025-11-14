/**
 * \file data-structures/graph.hpp
 **/
#ifndef OTHER_CORE_DATA_STRUCTURES_GRAPH_HPP
#define OTHER_CORE_DATA_STRUCTURES_GRAPH_HPP

#include <ranges>

#include "math/dynamic_matrix.hpp"

#include "data-structures/arena_vector.hpp"

namespace other {

  template <typename T>
  class graph {
   public:
    graph() = default;
    ~graph() { clear(); }

    void clear() {
      nodes.clear();
    }

    uint64_t add_node(T&& value) {
      return nodes.emplace_back(node{ get_next_id(), std::move(value) }).id;
    }

    void remove_node(uint64_t id) {
      auto itr = std::ranges::find_if(nodes, [id](const auto& n) { return n.id == id; });
      if (itr != nodes.end()) {
        nodes.erase(itr);
      }
    }

    T* ptr_to_node_value(uint64_t id) {
      auto itr = std::ranges::find_if(nodes, [id](const auto& n) { return n.id == id; });
      if (itr != nodes.end()) {
        return &itr->value;
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

   private:
    struct node {
      uint64_t id = 0;
      T value;

      constexpr auto operator<=>(const node& other) const = default;
    };
    std::vector<node> nodes;
    std::map<natural_t, std::vector<natural_t>> adjacency_list;

    uint64_t id_counter = 0;
    uint64_t get_next_id() {
      return ++id_counter;
    }
  };

}  // namespace other

#endif  // OTHER_CORE_DATA_STRUCTURES_GRAPH_HPP