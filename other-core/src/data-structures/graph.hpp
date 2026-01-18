/**
 * \file data-structures/graph.hpp
 **/
#ifndef OTHER_CORE_DATA_STRUCTURES_GRAPH_HPP
#define OTHER_CORE_DATA_STRUCTURES_GRAPH_HPP

#include <map>
#include <vector>

#include "core/defines.hpp"

namespace other {

  template <typename T>
  class graph {
   public:
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

   private:
    struct node {
      uint64_t id = 0;
      T value;

      constexpr auto operator<=>(const node& other) const = default;
    };
    std::map<uint64_t, node> nodes;
    std::map<natural_t, std::vector<natural_t>> adjacency_list;

    uint64_t id_counter = 0;
    uint64_t get_next_id() {
      return ++id_counter;
    }
  };

}  // namespace other

#endif  // OTHER_CORE_DATA_STRUCTURES_GRAPH_HPP