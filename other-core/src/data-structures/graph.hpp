/**
 * \file data-structures/graph.hpp
 **/
#ifndef OTHER_CORE_DATA_STRUCTURES_GRAPH_HPP
#define OTHER_CORE_DATA_STRUCTURES_GRAPH_HPP

#include <map>
#include <set>
#include <vector>

#include "core/defines.hpp"
#include "core/ref.hpp"
#include "math/dynamic_matrix.hpp"

namespace other {

  template <typename T>
  class graph {
   public:
    struct topology {
      bool has_cycles = false;
      std::vector<natural_t> sorted_node_ids;
    };

    graph() = default;
    ~graph() { clear(); }

    void clear() {
      nodes.clear();
    }

    bool empty() const {
      return nodes.empty();
    }

    size_t size() const {
      return nodes.size();
    }

    auto begin() { return nodes.begin(); }
    auto begin() const { return nodes.begin(); }
    auto end() { return nodes.end(); }
    auto end() const { return nodes.end(); }

    uint64_t add_node(T&& value) {
      auto id = get_next_id();
      auto [itr, success] = nodes.emplace(id, node{ id, std::move(value) });
      if (!success) {
        return 0;
      }

      return itr->first;
    }

    void remove_node(uint64_t id) {
      if (auto itr = nodes.find(id); itr != nodes.end()) {
        nodes.erase(itr);
      }
    }

    T* ptr_to_node_value(uint64_t id) {
      if (auto itr = nodes.find(id); itr != nodes.end()) {
        return &itr->second.value;
      }
      return nullptr;
    }

    std::vector<uint64_t> get_all_node_ids() const {
      std::vector<uint64_t> ids;
      for (const auto& [id, node] : nodes) {
        ids.push_back(id);
      }
      return ids;
    }

    template <typename Pred>
      requires requires(Pred p, T t) { { p(t) } -> std::same_as<bool>; }
    T* find_item(Pred&& predicate) {
      for (auto& [id, node] : nodes) {
        if (predicate(node.value)) {
          return &node.value;
        }
      }
      return nullptr;
    }

    template <typename Pred>
      requires requires(Pred p, T t) { { p(t) } -> std::same_as<bool>; }
    const T* find_item(Pred&& predicate) const {
      for (const auto& [id, node] : nodes) {
        if (predicate(node.value)) {
          return &node.value;
        }
      }
      return nullptr;
    }

    template <typename Func>
      requires requires(Func f, T t) { f(t); }
    void for_each_node(Func&& func) {
      for (auto& [id, node] : nodes) {
        func(node.value);
      }
    }

    void add_edge(uint64_t from_id, uint64_t to_id, real_t weight = 1.0) {
      if (nodes.find(from_id) == nodes.end() || nodes.find(to_id) == nodes.end()) {
        return;
      }
      OTHER_ASSERT(adjacency_matrix != nullptr, "Adjacency matrix is not initialized for graph.");
      OTHER_ASSERT(from_id < adjacency_matrix->cols && to_id < adjacency_matrix->rows, "Node IDs exceed adjacency matrix dimensions.");
      (*adjacency_matrix)(from_id, to_id) = weight;
    }

    void remove_edge(uint64_t from_id, uint64_t to_id) {
      if (nodes.find(from_id) == nodes.end() || nodes.find(to_id) == nodes.end()) {
        return;
      }
      OTHER_ASSERT(adjacency_matrix != nullptr, "Adjacency matrix is not initialized for graph.");
      OTHER_ASSERT(from_id < adjacency_matrix->cols && to_id < adjacency_matrix->rows, "Node IDs exceed adjacency matrix dimensions.");
      (*adjacency_matrix)(from_id, to_id) = 0.0;
    }

    topology topological_sort() const {
      OTHER_ASSERT(adjacency_matrix != nullptr, "Adjacency matrix is not initialized for graph.");
      PROFILE_SECTION("graph<T>::topological_sort");

      topology topo;
      if (nodes.empty()) {
        return topo;
      }

      topo.sorted_node_ids.reserve(nodes.size());

      std::map<natural_t, natural_t> in_degree;
      std::set<natural_t> no_incoming_edges;

      for (const auto& node_id : get_all_node_ids()) {
        for (const auto& other_id : get_all_node_ids()) {
          OTHER_ASSERT(other_id < adjacency_matrix->cols && node_id < adjacency_matrix->rows, "Node IDs exceed adjacency matrix dimensions.");
          if ((*adjacency_matrix)(other_id, node_id) != 0.0) {
            in_degree[node_id]++;
          }
        }
        if (in_degree[node_id] == 0) {
          no_incoming_edges.insert(node_id);
        }
      }

      /// deep copy so we can modify it as we remove edges
      ref<dynamic_matrix<real_t>> adj_copy = dynamic_matrix<real_t>::create_matrix(*adjacency_matrix);
      while (!no_incoming_edges.empty()) {
        natural_t current = *no_incoming_edges.begin();
        no_incoming_edges.erase(no_incoming_edges.begin());
        topo.sorted_node_ids.push_back(current);

        for (const auto& neighbor_id : get_all_node_ids()) {
          OTHER_ASSERT(neighbor_id < adj_copy->cols && current < adj_copy->rows, "Node IDs exceed adjacency matrix dimensions.");
          if ((*adj_copy)(current, neighbor_id) != 0.0) {
            (*adj_copy)(current, neighbor_id) = 0.0;
            in_degree[neighbor_id]--;
            if (in_degree[neighbor_id] == 0) {
              no_incoming_edges.insert(neighbor_id);
            }
          }
        }
      }

      topo.has_cycles = std::ranges::any_of(in_degree, [](auto degree) { return degree.second > 0; });
      if (topo.has_cycles) {
        topo.sorted_node_ids.clear();
      }

      return topo;
    }

   private:
    struct node {
      uint64_t id = 0;
      T value;

      constexpr auto operator<=>(const node& other) const = default;
    };
    std::map<uint64_t, node> nodes;
    ref<matrix_nxm<real_t>> adjacency_matrix = nullptr;

    uint64_t id_counter = 0;
    uint64_t get_next_id() {
      return ++id_counter;
    }
  };

}  // namespace other

#endif  // OTHER_CORE_DATA_STRUCTURES_GRAPH_HPP