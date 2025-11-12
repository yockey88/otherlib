/**
 * \file scripting/execution_node.cpp
 **/
#include "scripting/execution_node.hpp"

#include "scripting/execution_graph.hpp"

namespace other {

  void execution_node::read_inputs_from_predecessors(execution_graph* graph) {
    OTHER_ASSERT(graph != nullptr, "Execution graph pointer is null in read_inputs_from_predecessors.");
    if (get_num_inputs() == 0) {
      return;
    }

    const auto& links = graph->links;
    auto this_in_links = links | std::views::filter([this](const auto& l) { return l.to.node_id == this->id; });
    for (const auto& l : this_in_links) {
      execution_graph::node& from_node = graph->get_node_by_id(l.from.node_id);

      natural_t from_pin_idx = from_node.exec_node->get_this_node_output_index(l.from.pin_index);
      natural_t to_pin_idx = get_this_node_input_index(l.to.pin_index);
      raw_register(to_pin_idx) = from_node.exec_node->raw_register(from_pin_idx);
    }
  }

}  // namespace other