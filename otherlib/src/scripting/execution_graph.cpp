/**
 * \file scripting/execution_graph.cpp
 **/
#include "scripting/execution_graph.hpp"

namespace other {

  void execution_graph::set_delta_time(float dt) {
    nodes.for_each_node([dt](auto& n) {
      n.exec_node->delta_time = dt;
    });
  }

  execution_graph::node& execution_graph::get_node_by_id(natural_t id) {
    node* n = nodes.ptr_to_node_value(id);
    if (n != nullptr) {
      return *n;
    }

    static node invalid_node = { 0, 0, "invalid" };
    CORE_LOG_ERROR("Node with ID {} not found in graph!", id);
    return invalid_node;
  }

  execution_graph::node& execution_graph::get_node_by_name(const std::string_view name) {
    natural_t name_hash = FNV(name);
    node* n = nodes.find_item([name_hash](const auto& n) { return n.name_hash == name_hash; });
    if (n != nullptr) {
      return *n;
    }

    static node invalid_node = { 0, 0, "invalid" };
    CORE_LOG_ERROR("Node with name '{}' not found in graph!", name);
    return invalid_node;
  }

  void execution_graph::connect_nodes(const std::string_view from_node, uint8_t from_pin_idx, const std::string_view to_node, uint8_t to_pin_idx) {
    auto* from_n = nodes.find_item([from_node](const auto& n) { return n.name == from_node; });
    if (from_n == nullptr) {
      CORE_LOG_ERROR("From node '{}' not found in graph!", from_node);
      return;
    }

    node* to_n = nodes.find_item([to_node](const auto& n) { return n.name == to_node; });
    if (to_n == nullptr) {
      CORE_LOG_ERROR("To node '{}' not found in graph!", to_node);
      return;
    }

    auto link_itr = std::ranges::find_if(links, [&](const auto& l) {
      return l.from.node_id == from_n->id && l.from.pin_index == from_pin_idx && l.to.node_id == to_n->id && l.to.pin_index == to_pin_idx;
    });
    if (link_itr != links.end()) {
      CORE_LOG_WARN("Link from node '{}' pin {} to node '{}' pin {} already exists, skipping connection.", from_node, from_pin_idx, to_node, to_pin_idx);
      return;
    }

    execution_link& l = links.emplace_back();
    l.from = { from_n->id, from_pin_idx };
    l.to = { to_n->id, to_pin_idx };
  }

  std::vector<natural_t> execution_graph::topological_sort() {
    if (nodes.empty()) {
      return {};
    }

    std::vector<natural_t> sorted;
    sorted.reserve(nodes.size());

    std::map<natural_t, natural_t> in_degree;

    std::set<natural_t> no_incoming_edges;
    auto all_node_ids = nodes.get_all_node_ids();
    for (const auto& node_id : all_node_ids) {
      auto incoming_edges = std::ranges::count_if(links, [&](const auto& l) {
        return l.to.node_id == node_id;
      });
      in_degree[node_id] = incoming_edges;
      if (incoming_edges == 0) {
        no_incoming_edges.insert(node_id);
      }
    }

    /// build list of edges to process
    std::map<natural_t, std::set<pin::address>> edges;
    for (const auto& l : links) {
      edges[l.from.node_id].insert({ l.to.node_id, l.to.pin_index });
    }

    while (!no_incoming_edges.empty()) {
      natural_t current = *no_incoming_edges.begin();
      no_incoming_edges.erase(no_incoming_edges.begin());
      sorted.push_back(current);

      auto& current_edges = edges[current];
      while (!current_edges.empty()) {
        natural_t neighbor = current_edges.begin()->node_id;
        current_edges.erase(current_edges.begin());

        in_degree[neighbor]--;
        if (in_degree[neighbor] == 0) {
          no_incoming_edges.insert(neighbor);
        }
      }
    }

    bool any_cycles = std::ranges::any_of(in_degree, [](auto degree) { return degree.second > 0; });
    if (any_cycles) {
      /// if cycle then return -1 to signal invalid graph
      return { static_cast<natural_t>(-1) };
    } else {
      return sorted;
    }
  }

}  // namespace other
