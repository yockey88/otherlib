/**
 * \file scripting/execution_graph.cpp
 **/
#include "scripting/execution_graph.hpp"

namespace other {

  natural_t execution_graph::add_node(const std::string_view name, scope<execution_node> exec_node) {
    natural_t new_id = nodes.size();
    nodes.push_back(node{ .id = new_id, .name_hash = FNV(name), .name = std::string{ name }, .exec_node = std::move(exec_node) });
    nodes.back().exec_node->id = new_id;
    return new_id;
  }

  execution_graph::node& execution_graph::get_node_by_id(natural_t id) {
    auto itr = std::ranges::find_if(nodes, [id](const auto& n) { return n.id == id; });
    if (itr != nodes.end()) {
      return *itr;
    }

    static node invalid_node = { 0, 0, "invalid", nullptr };
    CORE_LOG_ERROR("Node with ID {} not found in graph!", id);
    return invalid_node;
  }

  execution_graph::node& execution_graph::get_node_by_name(const std::string_view name) {
    natural_t name_hash = FNV(name);
    auto itr = std::ranges::find_if(nodes, [name_hash](const auto& n) { return n.name_hash == name_hash; });
    if (itr != nodes.end()) {
      return *itr;
    }

    static node invalid_node = { 0, 0, "invalid", nullptr };
    CORE_LOG_ERROR("Node with name '{}' not found in graph!", name);
    return invalid_node;
  }

  void execution_graph::connect_nodes(const std::string_view from_node, uint8_t from_pin_idx, const std::string_view to_node, uint8_t to_pin_idx) {
    auto from_itr = std::ranges::find_if(nodes, [from_node](const auto& n) { return n.name == from_node; });
    if (from_itr == nodes.end()) {
      CORE_LOG_ERROR("From node '{}' not found in graph!", from_node);
      return;
    }

    auto to_itr = std::ranges::find_if(nodes, [to_node](const auto& n) { return n.name == to_node; });
    if (to_itr == nodes.end()) {
      CORE_LOG_ERROR("To node '{}' not found in graph!", to_node);
      return;
    }

    auto link_itr = std::ranges::find_if(links, [&](const auto& l) {
      return l.from.node_id == from_itr->id && l.from.pin_index == from_pin_idx && l.to.node_id == to_itr->id && l.to.pin_index == to_pin_idx;
    });
    if (link_itr != links.end()) {
      CORE_LOG_WARN("Link from node '{}' pin {} to node '{}' pin {} already exists, skipping connection.", from_node, from_pin_idx, to_node, to_pin_idx);
      return;
    }

    execution_link& l = links.emplace_back();
    l.from = { from_itr->id, from_pin_idx };
    l.to = { to_itr->id, to_pin_idx };
  }

  std::vector<natural_t> execution_graph::topological_sort() {
    if (nodes.empty()) {
      return {};
    }

    std::vector<natural_t> sorted;
    sorted.reserve(nodes.size());

    std::vector<natural_t> in_degree;
    in_degree.resize(nodes.size(), 0);

    std::set<natural_t> no_incoming_edges;
    for (natural_t n = 0; n < nodes.size(); ++n) {
      for (const auto& l : links) {
        if (l.from.node_id == n) {
          in_degree[l.to.node_id]++;
        }
      }
      if (in_degree[n] == 0) {
        no_incoming_edges.insert(n);
      }
    }

    /// build list of edges to process
    std::map<natural_t, std::set<natural_t>> edges;
    for (const auto& l : links) {
      edges[l.from.node_id].insert(l.to.node_id);
    }

    while (!no_incoming_edges.empty()) {
      natural_t current = *no_incoming_edges.begin();
      no_incoming_edges.erase(no_incoming_edges.begin());
      sorted.push_back(current);

      auto& current_edges = edges[current];
      while (!current_edges.empty()) {
        natural_t neighbor = *current_edges.begin();
        current_edges.erase(current_edges.begin());

        in_degree[neighbor]--;
        if (in_degree[neighbor] == 0) {
          no_incoming_edges.insert(neighbor);
        }
      }
    }

    bool any_cycles = std::ranges::any_of(in_degree, [](natural_t degree) { return degree > 0; });
    if (any_cycles) {
      /// if cycle then return -1 to signal invalid graph
      return { static_cast<natural_t>(-1) };
    } else {
      return sorted;
    }
  }

}  // namespace other
