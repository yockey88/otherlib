/**
 * \file runtime-dev/behavior_tree.hpp
 **/
#ifndef OTHER_RUNTIME_DEV_BEHAVIOR_TREE_HPP
#define OTHER_RUNTIME_DEV_BEHAVIOR_TREE_HPP

#include <concepts>

#include "object/transform.hpp"

#include "scripting/execution_graph.hpp"

namespace other {

  struct behavior_tree : public execution_graph {
    behavior_tree();
    virtual ~behavior_tree() = default;

    void set_input_transform(const transform& t);
    transform get_output_transform();

    transform execute_tree();

   private:
    natural_t in_node_id = 0;
    natural_t out_node_id = 0;
  };

}  // namespace other

#endif  // OTHER_RUNTIME_DEV_BEHAVIOR_TREE_HPP