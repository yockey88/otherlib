/**
 * \file data-structures/graph.hpp
 **/
#ifndef OTHER_CORE_DATA_STRUCTURES_GRAPH_HPP
#define OTHER_CORE_DATA_STRUCTURES_GRAPH_HPP

#include <map>
#include <ranges>
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
      ostd::vector<natural_t> sorted_node_ids;
    };

    graph()
        : adjacency_matrix(make_ref<matrix_nxm<real_t>>(0, 0)) {}
    ~graph() { clear(); }
    graph(graph&& other)
        : nodes(std::move(other.nodes)),
          adjacency_matrix(std::move(other.adjacency_matrix)) {}
    graph& operator=(graph&& other) {
      if (this != &other) {
        nodes = std::move(other.nodes);
        adjacency_matrix = std::move(other.adjacency_matrix);
      }
      return *this;
    }
    graph(const graph& g) = delete;
    graph& operator=(const graph& g) = delete;

    natural_t id_to_idx(natural_t id) const {
      if (auto itr = std::ranges::find(nodes, id, &node::id); itr != nodes.end()) {
        return itr->index;
      }
      OTHER_ASSERT(false, "Node ID {} not found in graph.", id);
      return 0;
    }

    void clear() {
      for (auto& n : *this) {
        if (n.value != nullptr) {
          arena_allocator<T>{}.free(n.value);
          n.value = nullptr;
        }
      }
      nodes.clear();
      adjacency_matrix = make_ref<matrix_nxm<real_t>>(0, 0);
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

    void remove_neighbors(natural_t node_id) {
      OTHER_ASSERT(adjacency_matrix != nullptr, "Adjacency matrix is not initialized for graph.");
      /// easiest way is to just remove all edges leaving this node
      for (const natural_t other_id : get_all_node_ids()) {
        remove_edge(node_id, other_id);
      }
    }

    ostd::vector<natural_t> get_neighbors(natural_t node_id) const {
      ostd::vector<natural_t> neighbors;
      if (adjacency_matrix == nullptr) {
        return neighbors;
      }
      for (const natural_t other_id : get_all_node_ids()) {
        if (!detail::epsilon_zero((*adjacency_matrix)(id_to_idx(node_id), id_to_idx(other_id)))) {
          neighbors.push_back(other_id);
        }
      }
      return neighbors;
    }

    natural_t add_node(T&& value) {
      auto id = get_next_id();

      node n = {
        .id = id,
        .index = nodes.size(),
        .value = arena_allocator<T>{}.allocate(std::move(value)),
      };
      nodes.push_back(std::move(n));

      const ref<matrix_nxm<real_t>> old_adjacency = adjacency_matrix;
      adjacency_matrix = make_ref<matrix_nxm<real_t>>(dynamic_matrix<real_t>::create_matrix(nodes.size(), nodes.size()));

      for (size_t i = 0; i < old_adjacency->rows; ++i) {
        for (size_t j = 0; j < old_adjacency->cols; ++j) {
          (*adjacency_matrix)(i, j) = (*old_adjacency)(i, j);
        }
      }

      return id;
    }

    void remove_node(natural_t id) {
      OTHER_ASSERT(adjacency_matrix != nullptr, "Adjacency matrix is not initialized for graph.");
      auto itr = std::ranges::find(nodes, id, &node::id);
      if (itr == nodes.end()) {
        CORE_LOG_ERROR("Node with ID {} not found in graph.", id);
        return;
      }

      size_t idx = itr->index;
      arena_allocator<T>{}.free(itr->value);
      itr->value = nullptr;
      nodes.erase(itr);

      const ref<matrix_nxm<real_t>> old_adjacency = adjacency_matrix;
      adjacency_matrix = make_ref<matrix_nxm<real_t>>(dynamic_matrix<real_t>::create_matrix(nodes.size(), nodes.size()));
      size_t new_i = 0;
      for (size_t i = 0; i < old_adjacency->rows; ++i) {
        if (i == idx) {
          continue;
        }
        size_t new_j = 0;
        for (size_t j = 0; j < old_adjacency->cols; ++j) {
          if (j == idx) {
            continue;
          }
          (*adjacency_matrix)(new_i, new_j) = (*old_adjacency)(i, j);
          new_j++;
        }
        new_i++;
      }

      for (size_t i = 0; i < nodes.size(); ++i) {
        nodes[i].index = i;
      }
    }

    T* ptr_to_node_value(natural_t id) {
      if (auto itr = std::ranges::find(nodes, id, &node::id); itr != nodes.end()) {
        return itr->value;
      }
      return nullptr;
    }

    ostd::vector<natural_t> get_all_node_ids() const {
      return nodes |
        std::views::transform(&node::id) |
        std::ranges::to<ostd::vector<natural_t>>();
    }

    template <typename Pred>
      requires requires(Pred p, T t) { { p(t) } -> std::same_as<bool>; }
    T* find_item(Pred&& predicate) {
      for (auto& node : nodes) {
        OTHER_ASSERT(node.value != nullptr, "Node value is nullptr");
        if (predicate(*node.value)) {
          return node.value;
        }
      }
      return nullptr;
    }

    template <typename Pred>
      requires requires(Pred p, T t) { { p(t) } -> std::same_as<bool>; }
    const T* find_item(Pred&& predicate) const {
      for (const auto& node : nodes) {
        OTHER_ASSERT(node.value != nullptr, "Node value is nullptr");
        if (predicate(*node.value)) {
          return node.value;
        }
      }
      return nullptr;
    }

    template <typename Func>
      requires requires(Func f, T t) { f(t); }
    void for_each_node(Func&& func) {
      for (auto& node : nodes) {
        OTHER_ASSERT(node.value != nullptr, "Node value is nullptr");
        func(*node.value);
      }
    }

    void add_edge(natural_t from_id, natural_t to_id, real_t weight = 1.0) {
      if (id_to_idx(from_id) >= adjacency_matrix->rows || id_to_idx(to_id) >= adjacency_matrix->cols) {
        return;
      }

      OTHER_ASSERT(adjacency_matrix != nullptr, "Adjacency matrix is not initialized for graph.");
      OTHER_ASSERT(id_to_idx(from_id) < adjacency_matrix->rows && id_to_idx(to_id) < adjacency_matrix->cols, "Node IDs exceed adjacency matrix dimensions.");
      (*adjacency_matrix)(id_to_idx(from_id), id_to_idx(to_id)) = weight;
    }

    void remove_edge(natural_t from_id, natural_t to_id) {
      if (id_to_idx(from_id) >= adjacency_matrix->rows || id_to_idx(to_id) >= adjacency_matrix->cols) {
        return;
      }
      OTHER_ASSERT(adjacency_matrix != nullptr, "Adjacency matrix is not initialized for graph.");
      OTHER_ASSERT(id_to_idx(from_id) < adjacency_matrix->rows && id_to_idx(to_id) < adjacency_matrix->cols, "Node IDs exceed adjacency matrix dimensions.");
      (*adjacency_matrix)(id_to_idx(from_id), id_to_idx(to_id)) = 0.0;
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
          OTHER_ASSERT(id_to_idx(other_id) < adjacency_matrix->cols && id_to_idx(node_id) < adjacency_matrix->rows, "Node IDs exceed adjacency matrix dimensions.");
          if (!detail::epsilon_zero((*adjacency_matrix)(id_to_idx(other_id), id_to_idx(node_id)))) {
            in_degree[node_id]++;
          }
        }
      }
      for (const auto& node_id : get_all_node_ids()) {
        if (in_degree[node_id] == 0) {
          no_incoming_edges.insert(node_id);
        }
      }

      /// deep copy so we can modify it as we remove edges
      ref<dynamic_matrix<real_t>> adj_copy = make_ref<dynamic_matrix<real_t>>(dynamic_matrix<real_t>::create_matrix(*adjacency_matrix));
      while (!no_incoming_edges.empty()) {
        natural_t current = *no_incoming_edges.begin();
        no_incoming_edges.erase(no_incoming_edges.begin());
        topo.sorted_node_ids.push_back(current);

        for (const auto& neighbor_id : get_all_node_ids()) {
          OTHER_ASSERT(id_to_idx(neighbor_id) < adj_copy->cols && id_to_idx(current) < adj_copy->rows, "Node IDs exceed adjacency matrix dimensions.");
          if (!detail::epsilon_zero((*adj_copy)(id_to_idx(current), id_to_idx(neighbor_id)))) {
            (*adj_copy)(id_to_idx(current), id_to_idx(neighbor_id)) = 0.0;
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

    std::string to_string() const {
      std::stringstream ss;
      for (const auto& node : nodes) {
        if (node.value == nullptr) {
          ss << std::format("[{}] = <nullptr>\n", node.id);
          continue;
        }

        if constexpr (requires { std::formatter<T>{}; }) {
          ss << std::format("[{}] = {}\n", node.id, *node.value);
        } else {
          ss << std::format("[{}] = <non-streamable value>\n", node.id);
        }
        for (const auto& other_id : get_all_node_ids()) {
          if (!detail::epsilon_zero((*adjacency_matrix)(id_to_idx(node.id), id_to_idx(other_id)))) {
            ss << std::format("  -> [{}] (weight: {:.3f})\n", other_id, (*adjacency_matrix)(id_to_idx(node.id), id_to_idx(other_id)));
          }
        }
      }
      return ss.str();
    }
    std::string to_matrix_string() const {
      return dynamic_matrix<real_t>::write_string(*adjacency_matrix);
    }

   private:
    struct node {
      natural_t id = 0;
      natural_t index = 0;
      T* value;

      constexpr auto operator<=>(const node& other) const = default;
    };
    ostd::vector<node> nodes;
    ref<matrix_nxm<real_t>> adjacency_matrix;

    natural_t id_counter = 0;
    natural_t get_next_id() {
      return ++id_counter;
    }
  };

}  // namespace other

#endif  // OTHER_CORE_DATA_STRUCTURES_GRAPH_HPP