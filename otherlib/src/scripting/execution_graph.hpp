/**
 * \file execution_graph.hpp
 **/
#ifndef OTHER_EXECUTION_GRAPH_HPP
#define OTHER_EXECUTION_GRAPH_HPP

#include <concepts>
#include <set>
#include <string>
#include <vector>

#include "core/defines.hpp"
#include "core/fnv.hpp"
#include "core/logger.hpp"
#include "core/scope.hpp"

#include "object/scene_object.hpp"

#include "scripting/execution_node.hpp"

#include "data-structures/graph.hpp"

namespace other {

  struct execution_graph {
    struct node {
      natural_t id;
      natural_t name_hash = 0;
      std::string name;

      execution_node* exec_node = nullptr;
    };

    virtual ~execution_graph() = default;

    template <typename T, typename... Args>
      requires std::constructible_from<T, Args...>
    natural_t add_node(const std::string_view name, Args&&... args) {
      natural_t new_id = nodes.add_node(node{
        .name_hash = FNV(name),
        .name = std::string(name),
        .exec_node = arena_allocator<T>{}.allocate(std::forward<Args>(args)...),
      });

      auto& n = get_node_by_id(new_id);
      CORE_LOG_DEBUG("Added node [{}] with ID {} to execution graph.", n.name, n.id);
      n.id = new_id;
      n.exec_node->id = new_id;

      return new_id;
    }

    void set_delta_time(float dt);

    node& get_node_by_id(natural_t id);
    node& get_node_by_name(const std::string_view name);

    void connect_nodes(const std::string_view from_node, uint8_t from_pin_idx, const std::string_view to_node, uint8_t to_pin_idx);

    std::vector<natural_t> topological_sort();

    graph<node> nodes;
    std::vector<execution_link> links;

   protected:
    template <typename T>
    T& get_node_as(natural_t id) {
      node& n = get_node_by_id(id);
      OTHER_ASSERT(n.exec_node != nullptr, "Execution node pointer is null for node ID {}.", id);

      T* casted_ptr = dynamic_cast<T*>(n.exec_node);
      OTHER_ASSERT(casted_ptr != nullptr, "Failed to cast execution node ID {} to requested type.", id);
      return *casted_ptr;
    }
  };

}  // namespace other

#endif  // OTHER_EXECUTION_GRAPH_HPP