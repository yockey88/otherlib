/**
 * \file execution_graph.hpp
 **/
#ifndef OTHER_EXECUTION_GRAPH_HPP
#define OTHER_EXECUTION_GRAPH_HPP

#include <set>
#include <string>
#include <vector>

#include "core/defines.hpp"
#include "core/fnv.hpp"
#include "core/logger.hpp"
#include "core/scope.hpp"

#include "scripting/execution_node.hpp"

namespace other {

  struct execution_graph {
    struct node {
      natural_t id;
      natural_t name_hash = 0;
      std::string name;

      scope<execution_node> exec_node = nullptr;
    };

    natural_t add_node(const std::string_view name, scope<execution_node> exec_node);
    node& get_node_by_id(natural_t id);
    node& get_node_by_name(const std::string_view name);

    void connect_nodes(const std::string_view from_node, uint8_t from_pin_idx, const std::string_view to_node, uint8_t to_pin_idx);

    std::vector<natural_t> topological_sort();

    std::vector<node> nodes;
    std::vector<execution_link> links;
  };

}  // namespace other

#endif  // OTHER_EXECUTION_GRAPH_HPP