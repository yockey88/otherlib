/**
 * \file scripting/execution_nodes/linear_algebra_nodes.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_EXECUTION_NODES_LINEAR_ALGEBRA_NODES_HPP
#define OTHERLIB_SCRIPTING_EXECUTION_NODES_LINEAR_ALGEBRA_NODES_HPP

#include "scripting/execution_node.hpp"

namespace other {

  struct add_vec2_node : public execution_node_impl<2, 1> {
    virtual ~add_vec2_node() = default;

    void execute_node() override {
      glm::vec2 a = read_input<0, glm::vec2>();
      glm::vec2 b = read_input<1, glm::vec2>();
      glm::vec2 result = a + b;
      write_output<0>(result);
    }
  };

  struct add_vec3_node : public execution_node_impl<2, 1> {
    virtual ~add_vec3_node() = default;

    void execute_node() override {
      glm::vec3 a = read_input<0, glm::vec3>();
      glm::vec3 b = read_input<1, glm::vec3>();
      glm::vec3 result = a + b;
      write_output<0>(result);
    }
  };

  struct add_vec4_node : public execution_node_impl<2, 1> {
    virtual ~add_vec4_node() = default;

    void execute_node() override {
      glm::vec4 a = read_input<0, glm::vec4>();
      glm::vec4 b = read_input<1, glm::vec4>();
      glm::vec4 result = a + b;
      write_output<0>(result);
    }
  };

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_EXECUTION_NODES_LINEAR_ALGEBRA_NODES_HPP