/**
 * \file scripting/execution_nodes/linear_algebra_nodes.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_EXECUTION_NODES_LINEAR_ALGEBRA_NODES_HPP
#define OTHERLIB_SCRIPTING_EXECUTION_NODES_LINEAR_ALGEBRA_NODES_HPP

#include "scripting/execution_node.hpp"

namespace other {

  struct add_vec2_node : public execution_node_impl<2, 1> {
    virtual ~add_vec2_node() = default;

    void execute_node() override;
  };

  struct add_vec3_node : public execution_node_impl<2, 1> {
    virtual ~add_vec3_node() = default;

    void execute_node() override;
  };

  struct add_vec4_node : public execution_node_impl<2, 1> {
    virtual ~add_vec4_node() = default;

    void execute_node() override;
  };

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_EXECUTION_NODES_LINEAR_ALGEBRA_NODES_HPP