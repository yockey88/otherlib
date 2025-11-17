/**
 * \file behavior_tree.cpp
 **/
#include "behavior_tree.hpp"

#include "scripting/execution_nodes/transform_nodes.hpp"

namespace other {

  behavior_tree::behavior_tree() {
    in_node_id = add_node<transform_source_node>("InputNode");
    out_node_id = add_node<transform_sink_node>("OutputNode");
  }

  void behavior_tree::set_input_transform(const transform& t) {
    auto& in_node = get_node_as<transform_source_node>(in_node_id);
    in_node.local_position = t.local_position;
    in_node.local_rotation = t.local_rotation_quat;
    in_node.local_scale = t.local_scale;
  }

  transform behavior_tree::get_output_transform() {
    auto& out_node = get_node_as<transform_sink_node>(out_node_id);
    transform t;
    t.local_position = out_node.position;
    t.local_rotation_quat = out_node.rotation;
    t.local_scale = out_node.scale;
    return t;
  }

  transform behavior_tree::execute_tree() {
    auto sorted_nodes = topological_sort();
    for (const auto& node_id : sorted_nodes) {
      auto& n = get_node_by_id(node_id);

      n.exec_node->read_inputs_from_predecessors(this);
      n.exec_node->execute_node();
    }
    return get_output_transform();
  }

}  // namespace other